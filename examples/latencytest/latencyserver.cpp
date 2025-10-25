#include "examplemessageprocessor.h"
#include "examplemessagerouter.h"
#include "latencyanalyzer.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostAddress>
#include <QSharedPointer>
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/ProtobufMessage>
#include <QtWebSocketProtobuf/WebSocketServer>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("WebSocket Protobuf RTT Server");
    app.setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("WebSocket Protobuf RTT Server");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption portOption(QStringList() << "p"
                                                << "port",
                                  "Server port (default: 8080)", "port", "8080");
    parser.addOption(portOption);
    parser.process(app);
    int port = parser.value(portOption).toInt();

#ifdef QT_NO_DEBUG
    QtWebSocketProtobuf::Logger::instance().setLogLevel(QtWebSocketProtobuf::LogLevel::Info);
#else
    QtWebSocketProtobuf::Logger::instance().setLogLevel(QtWebSocketProtobuf::LogLevel::Debug);
#endif

    auto router = QSharedPointer<ExampleMessageRouter>::create();
    auto processor = QSharedPointer<ExampleMessageProcessor>::create();
    router->registerProcessor(processor, {example::MessageType::CONNACK, example::MessageType::LATENCY_TEST});
    QtWebSocketProtobuf::WebSocketServer server(router.data());

    server.setSessionConnectedCallback([&server](const QString& sessionId) {
        LOG_INFO(QString("Session connected: %1").arg(sessionId));
        example::ConnAckMessage connAckMsg;
        auto messagePtr = QtWebSocketProtobuf::createMessage(connAckMsg, example::MessageType::CONNACK, sessionId);
        qint64 result = server.sendMessage(messagePtr);
        if (result > 0) {
            LOG_DEBUG(QString("CONNACK message sent (messageId: %1, sessionId: %2)")
                          .arg(messagePtr->messageId())
                          .arg(sessionId));
        } else {
            LOG_ERROR(QString("Failed to send CONNACK message (sessionId: %1)").arg(sessionId));
        }
    });
    server.setSessionDisconnectedCallback([](const QString& sessionId, const QString& reason) {
        LOG_INFO(QString("Session %1 disconnected: %2").arg(sessionId).arg(reason));
    });

    processor->setLatencyTestCallback(
        [&server](const example::LatencyTestMessage& message, const QString& sessionId, quint32 messageId) {
            if (sessionId.isEmpty()) {
                LOG_WARNING("Session ID is empty, cannot send echo response!");
                return;
            }

            // 记录服务端接收时间戳
            qint64 serverReceiveTime = getHighResolutionTimestamp();

            example::LatencyTestMessage latencyTestResponse = message;
            latencyTestResponse.set_request_message_id(messageId);
            latencyTestResponse.set_server_receive_timestamp(serverReceiveTime);

            // 记录服务端发送时间戳
            qint64 serverSendTime = getHighResolutionTimestamp();
            latencyTestResponse.set_server_send_timestamp(serverSendTime);

            auto messagePtr =
                QtWebSocketProtobuf::createMessage(latencyTestResponse, example::MessageType::LATENCY_TEST, sessionId);
            server.sendMessage(messagePtr);
        });

    if (!server.start(QHostAddress::Any, port)) {
        return 1;
    }
    LOG_INFO(QString("RTT Server started on port %1").arg(port));
    LOG_INFO("Press Ctrl+C to quit");
    return app.exec();
}
