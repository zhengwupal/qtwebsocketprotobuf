#include "websocketserver.h"
#include "../utils/logger.h"
#include "abstractmessagerouter.h"
#include "broadcastresult.h"
#include "message.h"
#include "sessionmanager.h"
#include "websocketserver_p.h"
#include <QHostAddress>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QWebSocket>
#include <QWebSocketServer>

namespace QtWebSocketProtobuf
{

WebSocketServer::WebSocketServer(AbstractMessageRouter* router, QObject* parent)
    : QObject(parent), d_ptr(new WebSocketServerPrivate(this, router))
{
    Q_D(WebSocketServer);
    if (!router) {
        LOG_ERROR("Cannot create WebSocketServer: router is null");
        return;
    }
    connect(d->server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
    connect(d->server, &QWebSocketServer::serverError, this, &WebSocketServer::onServerError);
}

WebSocketServer::~WebSocketServer()
{
    stop();
}

bool WebSocketServer::start(const QHostAddress& address, quint16 port)
{
    Q_D(WebSocketServer);
    if (d->server->isListening()) {
        LOG_WARNING("Server is already running");
        return true;
    }

    bool success = d->server->listen(address, port);
    if (success) {
        LOG_DEBUG(QString("Server started on %1:%2").arg(address.toString()).arg(port));
    } else {
        QString errorMessage = d->server->errorString();
        LOG_ERROR(QString("Failed to start server: %1").arg(errorMessage));

        emit serverError(errorMessage);
        if (d->serverErrorCallback) {
            d->serverErrorCallback(errorMessage);
        }
    }

    return success;
}

void WebSocketServer::stop()
{
    Q_D(WebSocketServer);
    if (!d->server->isListening()) {
        return;
    }

    LOG_DEBUG("Stopping server");
    const QStringList sessionIds = d->sessionManager->getAllSessionIds();
    for (const QString& sessionId : sessionIds) {
        d->sessionManager->disconnectSession(sessionId, "Server shutdown");
    }

    d->server->close();
    LOG_DEBUG("Server stopped");
}

bool WebSocketServer::isRunning() const
{
    Q_D(const WebSocketServer);
    return d->server->isListening();
}

quint16 WebSocketServer::serverPort() const
{
    Q_D(const WebSocketServer);
    return d->server->serverPort();
}

QHostAddress WebSocketServer::serverAddress() const
{
    Q_D(const WebSocketServer);
    return d->server->serverAddress();
}

AbstractMessageRouter* WebSocketServer::messageRouter()
{
    Q_D(WebSocketServer);
    return d->messageRouter;
}

void WebSocketServer::setSessionConnectedCallback(SessionConnectedCallback callback)
{
    Q_D(WebSocketServer);
    d->sessionConnectedCallback = callback;
}

void WebSocketServer::setSessionDisconnectedCallback(SessionDisconnectedCallback callback)
{
    Q_D(WebSocketServer);
    d->sessionDisconnectedCallback = callback;
}

void WebSocketServer::setServerErrorCallback(ServerErrorCallback callback)
{
    Q_D(WebSocketServer);
    d->serverErrorCallback = callback;
}

void WebSocketServer::onNewConnection()
{
    Q_D(WebSocketServer);
    QWebSocket* socket = d->server->nextPendingConnection();
    if (!socket) {
        LOG_ERROR("Failed to get pending connection");
        return;
    }

    QString sessionId;
    bool isRestored = false;

    QUrl requestUrl = socket->requestUrl();
    if (requestUrl.hasQuery()) {
        QUrlQuery query(requestUrl.query());
        QString requestedSessionId = query.queryItemValue("session_id");
        if (!requestedSessionId.isEmpty()) {
            if (d->sessionManager->restoreSession(socket, requestedSessionId)) {
                sessionId = requestedSessionId;
                isRestored = true;
                LOG_DEBUG(QString("Session %1 restored from URL parameter").arg(sessionId));
            } else {
                sessionId = d->sessionManager->createSession(socket);
                LOG_DEBUG(QString("Failed to restore session %1, created new session %2")
                              .arg(requestedSessionId)
                              .arg(sessionId));
            }
        }
    }

    if (sessionId.isEmpty()) {
        sessionId = d->sessionManager->createSession(socket);
    }

    LOG_DEBUG(QString("Session %1 %2 from %3:%4")
                  .arg(sessionId)
                  .arg(isRestored ? "restored" : "created")
                  .arg(socket->peerAddress().toString())
                  .arg(socket->peerPort()));

    connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onSocketDisconnected);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::onBinaryMessageReceived);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
            &WebSocketServer::onSocketError);

    emit sessionConnected(sessionId);
    if (d->sessionConnectedCallback) {
        d->sessionConnectedCallback(sessionId);
    }
}

void WebSocketServer::onSocketDisconnected()
{
    Q_D(WebSocketServer);
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) {
        LOG_ERROR("Invalid socket disconnected");
        return;
    }

    QString sessionId = d->sessionManager->getSessionId(socket);
    if (sessionId.isEmpty()) {
        LOG_ERROR("Could not find session for disconnected socket");
        socket->deleteLater();
        return;
    }

    QString reason = socket->closeReason();
    LOG_DEBUG(
        QString("Session %1 disconnected: %2").arg(sessionId).arg(reason.isEmpty() ? "Connection closed" : reason));

    emit sessionDisconnected(sessionId, reason);
    if (d->sessionDisconnectedCallback) {
        d->sessionDisconnectedCallback(sessionId, reason);
    }

    d->sessionManager->removeSession(socket);
    socket->deleteLater();
}

void WebSocketServer::onBinaryMessageReceived(const QByteArray& message)
{
    Q_D(WebSocketServer);
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) {
        LOG_ERROR("Received binary message from invalid socket");
        return;
    }

    LOG_DEBUG(QString("Received binary message (%1 bytes)").arg(message.size()));
    d->messageRouter->processMessage(message);

    QString sessionId = d->sessionManager->getSessionId(socket);
    if (!sessionId.isEmpty()) {
        d->sessionManager->updateSessionActivity(sessionId);
    }
}

void WebSocketServer::onSocketError(QAbstractSocket::SocketError error)
{
    Q_D(WebSocketServer);
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) {
        LOG_ERROR("Socket error from invalid socket");
        return;
    }

    QString sessionId = d->sessionManager->getSessionId(socket);
    QString errorString = socket->errorString();

    if (error == QAbstractSocket::RemoteHostClosedError) {
        LOG_DEBUG(
            QString("Received RemoteHostClosedError for session %1").arg(sessionId.isEmpty() ? "unknown" : sessionId));
    } else {
        LOG_ERROR(QString("Socket error from session %1: %2 (code: %3)")
                      .arg(sessionId.isEmpty() ? "unknown" : sessionId)
                      .arg(errorString)
                      .arg(static_cast<int>(error)));
    }
}

void WebSocketServer::onServerError(QWebSocketProtocol::CloseCode closeCode)
{
    Q_D(WebSocketServer);
    QString errorMessage = QString("Server error: %1 (code %2)").arg(d->server->errorString()).arg(closeCode);
    LOG_ERROR(errorMessage);
    emit serverError(errorMessage);
    if (d->serverErrorCallback) {
        d->serverErrorCallback(errorMessage);
    }
}

bool WebSocketServer::isSessionConnected(const QString& sessionId) const
{
    Q_D(const WebSocketServer);
    return d->sessionManager->hasSession(sessionId);
}

int WebSocketServer::sessionCount() const
{
    Q_D(const WebSocketServer);
    return d->sessionManager->sessionCount();
}

qint64 WebSocketServer::sendBinaryMessage(const QString& sessionId, const QByteArray& data)
{
    Q_D(WebSocketServer);
    return d->sessionManager->sendBinaryMessage(sessionId, data);
}

qint64 WebSocketServer::sendMessage(MessagePtr message)
{
    Q_D(WebSocketServer);
    if (!message) {
        LOG_ERROR("Cannot send null message");
        return -1;
    }

    QString sessionId = message->sessionId();
    if (sessionId.isEmpty()) {
        LOG_ERROR("Cannot send message: session ID is empty");
        return -1;
    }

    quint32 messageId = d->sessionManager->getNextMessageId(sessionId);
    message->setMessageId(messageId);
    QByteArray data = d->messageRouter->serializeMessage(message);
    if (data.isEmpty()) {
        LOG_ERROR("Failed to serialize message");
        return -1;
    }

    qint64 result = d->sessionManager->sendBinaryMessage(sessionId, data);
    if (result > 0) {
        LOG_DEBUG(QString("Message sent (messageId: %1, sessionId: %2)").arg(messageId).arg(sessionId));
    }

    return result;
}

void WebSocketServer::disconnectSession(const QString& sessionId, const QString& reason)
{
    Q_D(WebSocketServer);
    d->sessionManager->disconnectSession(sessionId, reason);
}

BroadcastResult WebSocketServer::broadcastBinaryMessage(const QByteArray& data)
{
    Q_D(WebSocketServer);
    return d->sessionManager->broadcastBinaryMessage(data);
}

void WebSocketServer::broadcastMessage(MessagePtr message)
{
    Q_D(WebSocketServer);
    if (!message) {
        LOG_ERROR("Cannot broadcast null message");
        return;
    }

    message->setSessionId(QString());
    QByteArray data = d->messageRouter->serializeMessage(message);
    if (data.isEmpty()) {
        LOG_ERROR("Failed to serialize broadcast message");
        return;
    }

    BroadcastResult result = d->sessionManager->broadcastBinaryMessage(data);
    if (result.successCount() > 0) {
        LOG_DEBUG(QString("Broadcast message sent to %1/%2 sessions, total bytes: %3")
                      .arg(result.successCount())
                      .arg(result.totalCount())
                      .arg(result.totalBytesSent()));
    } else {
        LOG_ERROR(QString("Failed to broadcast message: %1").arg(result.error()));
    }

    if (result.hasFailedSessions()) {
        LOG_WARNING(
            QString("Failed to send broadcast message to sessions: %1").arg(result.failedSessions().join(", ")));
    }
}

}  // namespace QtWebSocketProtobuf
