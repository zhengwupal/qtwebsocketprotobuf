#include "examplemessageprocessor.h"
#include "examplemessagerouter.h"
#include "latencyanalyzer.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QList>
#include <QSharedPointer>
#include <QTimer>
#include <QUrl>
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/ProtobufMessage>
#include <QtWebSocketProtobuf/WebSocketClient>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("WebSocket Protobuf RTT Client");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("WebSocket Protobuf RTT Client");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption hostOption(QStringList() << "host", "Server host (default: localhost)", "host", "localhost");
    parser.addOption(hostOption);
    QCommandLineOption portOption(QStringList() << "port", "Server port (default: 8080)", "port", "8080");
    parser.addOption(portOption);
    QCommandLineOption dataSizeOption(QStringList() << "data-size", "Test data size in bytes (default: 0)", "data-size",
                                      "0");
    parser.addOption(dataSizeOption);
    QCommandLineOption intervalOption(QStringList() << "interval", "Timer interval in milliseconds (default: 100)",
                                      "interval", "100");
    parser.addOption(intervalOption);
    parser.process(app);
    QString host = parser.value(hostOption);
    int port = parser.value(portOption).toInt();
    int dataSize = parser.value(dataSizeOption).toInt();
    int interval = parser.value(intervalOption).toInt();

#ifdef QT_NO_DEBUG
    QtWebSocketProtobuf::Logger::instance().setLogLevel(QtWebSocketProtobuf::LogLevel::Info);
#else
    QtWebSocketProtobuf::Logger::instance().setLogLevel(QtWebSocketProtobuf::LogLevel::Debug);
#endif

    auto router = QSharedPointer<ExampleMessageRouter>::create();
    auto processor = QSharedPointer<ExampleMessageProcessor>::create();
    router->registerProcessor(processor, {example::MessageType::CONNACK, example::MessageType::LATENCY_TEST});
    QtWebSocketProtobuf::WebSocketClient client(router.data());

    client.setConnectionTimeoutCallback([&app]() {
        LOG_ERROR("Connection timeout, server not responding");
        app.quit();
    });

    QObject::connect(processor.data(), &ExampleMessageProcessor::connAckReceived, [&client](const QString& sessionId) {
        client.setSessionId(sessionId);
        LOG_DEBUG(QString("Session ID received and saved: %1").arg(sessionId));
    });

    QTimer* latencyTimer = new QTimer(&client);
    latencyTimer->setInterval(interval);
    QObject::connect(latencyTimer, &QTimer::timeout, [&client, dataSize]() {
        if (client.sessionId().isEmpty()) {
            LOG_WARNING("Session ID not received yet, skipping latency test");
            return;
        }

        example::LatencyTestMessage msg;
        msg.set_client_send_timestamp(getHighResolutionTimestamp());
        if (dataSize > 0) {
            msg.set_data(QByteArray(dataSize, 'a').toStdString());
        }
        auto messagePtr =
            QtWebSocketProtobuf::createMessage(msg, example::MessageType::LATENCY_TEST, client.sessionId());
        client.sendMessage(messagePtr);
    });

    LatencyAnalyzer latencyAnalyzer(1000);  // 最多保存1000个样本

    processor->setLatencyTestCallback(
        [&latencyAnalyzer](const example::LatencyTestMessage& message, const QString&, quint32) {
            qint64 now = getHighResolutionTimestamp();
            qint64 rttMicroseconds = now - message.client_send_timestamp();

            // 计算纯网络RTT
            qint64 networkRttMicroseconds = 0;
            if (message.server_receive_timestamp() > 0 && message.server_send_timestamp() > 0) {
                qint64 serverProcessingTime = message.server_send_timestamp() - message.server_receive_timestamp();
                networkRttMicroseconds = rttMicroseconds - serverProcessingTime;
            }

            latencyAnalyzer.addSample(rttMicroseconds);

            LOG_INFO(QString("RTT: %1μs | Network RTT: %2μs | %3")
                         .arg(rttMicroseconds)
                         .arg(networkRttMicroseconds)
                         .arg(latencyAnalyzer.getStatisticsString()));
        });

    client.setStateChangedCallback(
        [&latencyTimer, &app, dataSize, interval](QAbstractSocket::SocketState newState, QAbstractSocket::SocketState) {
            if (newState == QAbstractSocket::ConnectedState) {
                LOG_INFO("Connected to server");
                LOG_INFO(QString("Data size: %1 bytes").arg(dataSize));
                LOG_INFO(QString("Timer interval: %1 ms").arg(interval));
                latencyTimer->start();
            } else if (newState == QAbstractSocket::UnconnectedState || newState == QAbstractSocket::ClosingState) {
                LOG_INFO("Disconnected from server");
                latencyTimer->stop();
                app.quit();
            }
        });

    QUrl url;
    url.setScheme("ws");
    url.setHost(host);
    url.setPort(port);
    LOG_INFO(QString("Connecting to %1:%2...").arg(host).arg(port));
    client.connectToServer(url);
    LOG_INFO("Press Ctrl+C to quit");
    return app.exec();
}
