#include "../messagetypeutils.h"
#include "examplemessageprocessor.h"
#include "examplemessagerouter.h"
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
    app.setApplicationName("WebSocket Protobuf Threaded Server Example");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("WebSocket Protobuf Threaded Server Example");
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
    router->registerProcessor(processor,
                              {example::MessageType::CONNACK, example::MessageType::STATUS, example::MessageType::ACK,
                               example::MessageType::ECHO, example::MessageType::GOODBYE});
    QtWebSocketProtobuf::ThreadedWebSocketServer server(router.data());

    QObject::connect(&server, &QtWebSocketProtobuf::ThreadedWebSocketServer::sendMessageResult,
                     [](const QtWebSocketProtobuf::SendResult& result) {
                         QString messageTypeStr = MessageTypeUtils::toString(result.messageType());
                         if (result.isSuccess()) {
                             LOG_INFO(QString("%1 message sent (messageId: %2, sessionId: %3)")
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
        &server, &QtWebSocketProtobuf::ThreadedWebSocketServer::sessionConnected, [&server](const QString& sessionId) {
            LOG_INFO(QString("Session connected: %1").arg(sessionId));
            example::ConnAckMessage connAckMsg;
            auto messagePtr = QtWebSocketProtobuf::createMessage(connAckMsg, example::MessageType::CONNACK, sessionId);
            server.sendMessage(messagePtr);
        });
    QObject::connect(&server, &QtWebSocketProtobuf::ThreadedWebSocketServer::sessionDisconnected,
                     [](const QString& sessionId, const QString& reason) {
                         LOG_INFO(QString("Session %1 disconnected: %2").arg(sessionId).arg(reason));
                     });

    processor->setEchoCallback(
        [&server](const example::EchoMessage& message, const QString& sessionId, quint32 messageId) {
            if (sessionId.isEmpty()) {
                LOG_WARNING("Session ID is empty, cannot send echo response!");
                return;
            }

            QString content = QString::fromStdString(message.content());
            LOG_INFO(QString("Received echo message: %1 (requestMessageId: %2, sessionId: %3)")
                         .arg(content)
                         .arg(messageId)
                         .arg(sessionId));

            example::AckMessage ackMsg;
            ackMsg.set_request_message_id(messageId);
            auto ackPtr = QtWebSocketProtobuf::createMessage(ackMsg, example::MessageType::ACK, sessionId);
            server.sendMessage(ackPtr);

            example::EchoMessage echoResponse;
            echoResponse.set_request_message_id(messageId);
            echoResponse.set_content(message.content());
            auto echoPtr = QtWebSocketProtobuf::createMessage(echoResponse, example::MessageType::ECHO, sessionId);
            server.sendMessage(echoPtr);
        });

    processor->setGoodbyeCallback([](const example::GoodbyeMessage& message) {
        LOG_INFO(QString("Client disconnecting, reason: %1").arg(QString::fromStdString(message.reason())));
    });

    LOG_INFO(QString("Starting server on port %1").arg(port));
    server.start(QHostAddress::Any, port);
    QObject::connect(&server, &QtWebSocketProtobuf::ThreadedWebSocketServer::serverStarted, [&port]() {
        LOG_INFO(QString("Server started on port %1").arg(port));
        LOG_INFO("Press Ctrl+C to quit");
    });

    return app.exec();
}
