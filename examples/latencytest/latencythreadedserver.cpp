#include "../messagetypeutils.h"
#include "examplemessageprocessor.h"
#include "examplemessagerouter.h"
#include "latencyanalyzer.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QSharedPointer>
#include <QThread>
#include <QTimer>
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/ProtobufMessage>
#include <QtWebSocketProtobuf/ThreadedWebSocketServer>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("WebSocket Protobuf Threaded RTT Server");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("WebSocket Protobuf Threaded RTT Server");
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
    QtWebSocketProtobuf::ThreadedWebSocketServer server(router.data());

    QObject::connect(&server, &QtWebSocketProtobuf::ThreadedWebSocketServer::sendMessageResult,
                     [](const QtWebSocketProtobuf::SendResult& result) {
                         QString messageTypeStr = MessageTypeUtils::toString(result.messageType());
                         if (result.isSuccess()) {
                             LOG_DEBUG(QString("%1 message sent (messageId: %2, sessionId: %3)")
                                           .arg(messageTypeStr)
                                           .arg(result.messageId())
                                           .arg(result.sessionId()));
                         } else {
                             LOG_ERROR(QString("Failed to send %1 message, %2 (sessionId: %3)")
                                           .arg(messageTypeStr)
                                           .arg(result.error())
                                           .arg(result.sessionId()));
                         }
                     });

    QObject::connect(
        &server, &QtWebSocketProtobuf::ThreadedWebSocketServer::sessionConnected, [&server](quint64 sessionId) {
            LOG_INFO(QString("Session connected: %1").arg(sessionId));
            example::ConnAckMessage connAckMsg;
            auto messagePtr = QtWebSocketProtobuf::createMessage(connAckMsg, example::MessageType::CONNACK, sessionId);
            server.sendMessage(messagePtr);
        });
    QObject::connect(&server, &QtWebSocketProtobuf::ThreadedWebSocketServer::sessionDisconnected,
                     [](quint64 sessionId, const QString& reason) {
                         LOG_INFO(QString("Session %1 disconnected: %2").arg(sessionId).arg(reason));
                     });

    processor->setLatencyTestCallback(
        [&server](const example::LatencyTestMessage& message, quint64 sessionId, quint32 messageId) {
            if (sessionId == 0) {
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

    QObject::connect(&server, &QtWebSocketProtobuf::ThreadedWebSocketServer::serverStarted, [&port]() {
        LOG_INFO(QString("RTT server started on port %1").arg(port));
        LOG_INFO("Press Ctrl+C to quit");
    });

    LOG_INFO(QString("Starting RTT server on port %1").arg(port));
    server.start(QHostAddress::Any, port);

    return app.exec();
}
