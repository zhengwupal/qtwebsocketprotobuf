#include "examplemessageprocessor.h"
#include "examplemessagerouter.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostAddress>
#include <QSharedPointer>
#include <QTimer>
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/ProtobufMessage>
#include <QtWebSocketProtobuf/WebSocketServer>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("WebSocket Protobuf Server Example");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("WebSocket Protobuf Server Example");
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
    QtWebSocketProtobuf::WebSocketServer server(router.data());

    server.setSessionConnectedCallback([&server](const QString& sessionId) {
        LOG_INFO(QString("Session connected: %1").arg(sessionId));
        example::ConnAckMessage connAckMsg;
        auto messagePtr = QtWebSocketProtobuf::createMessage(connAckMsg, example::MessageType::CONNACK, sessionId);
        qint64 result = server.sendMessage(messagePtr);
        if (result > 0) {
            LOG_INFO(QString("CONNACK message sent (messageId: %1)").arg(messagePtr->messageId()));
        } else {
            LOG_ERROR(QString("Failed to send CONNACK message (sessionId: %1)").arg(sessionId));
        }
    });
    server.setSessionDisconnectedCallback([](const QString& sessionId, const QString& reason) {
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
            qint64 ackResult = server.sendMessage(ackPtr);
            if (ackResult > 0) {
                LOG_INFO(QString("Ack message sent (requestMessageId: %1, messageId: %2)")
                             .arg(messageId)
                             .arg(ackPtr->messageId()));
            } else {
                LOG_ERROR("Failed to send Ack message");
            }

            example::EchoMessage echoResponse;
            echoResponse.set_request_message_id(messageId);
            echoResponse.set_content(message.content());
            auto messagePtr = QtWebSocketProtobuf::createMessage(echoResponse, example::MessageType::ECHO, sessionId);
            qint64 echoResult = server.sendMessage(messagePtr);
            if (echoResult > 0) {
                LOG_INFO(QString("Echo response sent (requestMessageId: %1, messageId: %2)")
                             .arg(messageId)
                             .arg(messagePtr->messageId()));
            } else {
                LOG_ERROR("Failed to send echo response");
            }
        });

    processor->setGoodbyeCallback([](const example::GoodbyeMessage& message) {
        LOG_INFO(QString("Client disconnecting, reason: %1").arg(QString::fromStdString(message.reason())));
    });

    if (!server.start(QHostAddress::Any, port)) {
        return 1;
    }
    LOG_INFO(QString("Server started on port %1").arg(port));
    LOG_INFO("Press Ctrl+C to quit");

    return app.exec();
}
