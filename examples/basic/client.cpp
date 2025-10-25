#include "../timeoutmanager.h"
#include "example_message.pb.h"
#include "examplemessageprocessor.h"
#include "examplemessagerouter.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QMap>
#include <QSharedPointer>
#include <QTimer>
#include <QUrl>
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/ProtobufMessage>
#include <QtWebSocketProtobuf/WebSocketClient>
#include <functional>

Q_DECLARE_METATYPE(example::AckMessage)

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("WebSocket Protobuf Client Example");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("WebSocket Protobuf Client Example");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption hostOption(QStringList() << "host", "Server host (default: localhost)", "host", "localhost");
    parser.addOption(hostOption);
    QCommandLineOption portOption(QStringList() << "port", "Server port (default: 8080)", "port", "8080");
    parser.addOption(portOption);
    QCommandLineOption sessionIdOption(QStringList() << "session-id", "Session ID to restore (optional)", "session-id");
    parser.addOption(sessionIdOption);
    parser.process(app);
    QString host = parser.value(hostOption);
    int port = parser.value(portOption).toInt();
    QString sessionId = parser.value(sessionIdOption);

#ifdef QT_NO_DEBUG
    QtWebSocketProtobuf::Logger::instance().setLogLevel(QtWebSocketProtobuf::LogLevel::Info);
#else
    QtWebSocketProtobuf::Logger::instance().setLogLevel(QtWebSocketProtobuf::LogLevel::Debug);
#endif

    auto router = QSharedPointer<ExampleMessageRouter>::create();
    auto processor = QSharedPointer<ExampleMessageProcessor>::create();
    router->registerProcessor(
        processor, {example::MessageType::CONNACK, example::MessageType::STATUS, example::MessageType::ECHO});
    QtWebSocketProtobuf::WebSocketClient client(router.data());

    client.setConnectionTimeoutCallback([&app]() {
        LOG_ERROR("Connection timeout, server not responding");
        app.quit();
    });

    TimeoutManager timeoutManager(50, &client);
    QObject::connect(&timeoutManager, &TimeoutManager::requestTimeout,
                     [](uint32_t requestId) { LOG_WARNING(QString("Request %1 timed out").arg(requestId)); });
    QObject::connect(&timeoutManager, &TimeoutManager::requestCompleted,
                     [](uint32_t requestId) { LOG_DEBUG(QString("Request %1 completed").arg(requestId)); });

    QObject::connect(processor.data(), &ExampleMessageProcessor::connAckReceived,
                     [&client, sessionId](const QString& receivedSessionId) {
                         client.setSessionId(receivedSessionId);
                         if (!sessionId.isEmpty() && sessionId == receivedSessionId) {
                             LOG_INFO(QString("Session successfully restored: %1").arg(receivedSessionId));
                         } else if (!sessionId.isEmpty() && sessionId != receivedSessionId) {
                             LOG_INFO(QString("Session restore failed (requested: %1, received: %2)")
                                          .arg(sessionId)
                                          .arg(receivedSessionId));
                         } else {
                             LOG_INFO(QString("New session created: %1").arg(receivedSessionId));
                         }
                     });
    QObject::connect(processor.data(), &ExampleMessageProcessor::ackReceived,
                     [&timeoutManager](const example::AckMessage& ack) {
                         uint32_t reqId = ack.request_message_id();
                         QVariant messageVariant = QVariant::fromValue(ack);
                         if (!timeoutManager.handleResponse(reqId, messageVariant)) {
                             LOG_WARNING(QString("Received ack message with unknown request ID: %1").arg(reqId));
                         }
                     });

    processor->setStatusCallback([](const example::StatusMessage& message) {
        QString details = QString::fromStdString(message.details());
        if (!details.isEmpty()) {
            LOG_INFO(QString("Received status message: %1 (Details: %2)")
                         .arg(QString::fromStdString(message.message()))
                         .arg(details));
        } else {
            LOG_INFO(QString("Received status message: %1").arg(QString::fromStdString(message.message())));
        }
    });
    processor->setEchoCallback(
        [](const example::EchoMessage& message, const QString& /*sessionId*/, quint32 /*messageId*/) {
            QString content = QString::fromStdString(message.content());
            uint32_t requestId = message.request_message_id();
            LOG_INFO(QString("Received echo message: %1 (messageId: %2)").arg(content).arg(requestId));
        });

    QTimer* echoTimer = nullptr;

    client.setStateChangedCallback([&echoTimer, &timeoutManager, &app, &client](QAbstractSocket::SocketState newState,
                                                                                QAbstractSocket::SocketState oldState) {
        Q_UNUSED(oldState);

        if (newState == QAbstractSocket::ConnectedState) {
            LOG_INFO("Connected to server");

            if (!echoTimer) {
                echoTimer = new QTimer(&client);
                echoTimer->setInterval(5000);
                echoTimer->start();

                QObject::connect(echoTimer, &QTimer::timeout, [&client, &timeoutManager]() {
                    if (client.sessionId().isEmpty()) {
                        LOG_WARNING("Session ID not received yet, skipping echo message");
                        return;
                    }

                    example::EchoMessage echoMsg;
                    echoMsg.set_content("Hello from client!");
                    auto messagePtr =
                        QtWebSocketProtobuf::createMessage(echoMsg, example::MessageType::ECHO, client.sessionId());

                    auto result = client.sendMessage(messagePtr);
                    if (result.isSuccess()) {
                        uint32_t messageId = result.messageId();
                        LOG_INFO(QString("Echo message sent (messageId: %1)").arg(messageId));

                        timeoutManager.addPendingRequest(
                            messageId,
                            [result](const QVariant& messageVariant) {
                                example::AckMessage ack = messageVariant.value<example::AckMessage>();
                                LOG_INFO(QString("Received ack message (messageId: %1)").arg(ack.request_message_id()));
                            },
                            3000);
                    } else {
                        LOG_ERROR("Failed to send echo message");
                    }
                });
            }
        } else if (newState == QAbstractSocket::UnconnectedState) {
            LOG_INFO("Disconnected from server");
            if (echoTimer) {
                echoTimer->stop();
            }
            timeoutManager.clearAllPendingRequests();
        } else if (newState == QAbstractSocket::ClosingState) {
            LOG_ERROR("Connection error occurred");
            if (echoTimer) {
                echoTimer->stop();
            }
            timeoutManager.clearAllPendingRequests();
            app.quit();
        }
    });

    QUrl url;
    url.setScheme("ws");
    url.setHost(host);
    url.setPort(port);

    LOG_INFO(QString("Connecting to %1:%2...").arg(host).arg(port));
    if (!sessionId.isEmpty()) {
        LOG_INFO(QString("Attempting to restore session: %1").arg(sessionId));
    }
    client.connectToServer(url, sessionId);
    LOG_INFO("Press Ctrl+C to quit");

    return app.exec();
}
