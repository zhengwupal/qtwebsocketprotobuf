#include "websocketclient.h"
#include "../utils/logger.h"
#include "message.h"
#include "sendresult.h"
#include "websocketclient_p.h"
#include <QMetaEnum>
#include <QMutexLocker>
#include <QUrlQuery>

namespace QtWebSocketProtobuf
{

WebSocketClient::WebSocketClient(AbstractMessageRouter* router, QObject* parent)
    : QObject(parent), d_ptr(new WebSocketClientPrivate(this, router))
{
    Q_D(WebSocketClient);

    if (!router) {
        LOG_ERROR("Cannot create WebSocketClient: router is null");
        return;
    }

    connect(d->webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(d->webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(d->webSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketClient::onBinaryMessageReceived);
    connect(d->webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
            &WebSocketClient::onError);

    d->reconnectTimer.setSingleShot(true);
    connect(&d->reconnectTimer, &QTimer::timeout, this, &WebSocketClient::onReconnectTimerTimeout);

    d->connectionTimeoutTimer.setSingleShot(true);
    connect(&d->connectionTimeoutTimer, &QTimer::timeout, this, &WebSocketClient::onConnectionTimeout);
}

WebSocketClient::~WebSocketClient()
{
    disconnectFromServer();
}

void WebSocketClient::connectToServer(const QUrl& url, const QString& sessionId)
{
    Q_D(WebSocketClient);
    if (d->state == QAbstractSocket::ConnectedState || d->state == QAbstractSocket::ConnectingState) {
        LOG_WARNING("Already connected or connecting to server");
        return;
    }

    d->serverUrl = url;
    setState(QAbstractSocket::ConnectingState);

    QUrl connectUrl = url;
    QString sessionIdToUse = sessionId.isEmpty() ? d->savedSessionId : sessionId;

    if (!sessionIdToUse.isEmpty()) {
        QUrlQuery query;
        if (connectUrl.hasQuery()) {
            query = QUrlQuery(connectUrl.query());
        }
        query.addQueryItem("session_id", sessionIdToUse);
        connectUrl.setQuery(query);
        LOG_DEBUG(QString("Connecting with session ID: %1").arg(sessionIdToUse));
    }

    LOG_DEBUG(QString("Connecting to server at %1").arg(connectUrl.toString()));

    if (d->connectionTimeoutEnabled && d->connectionTimeoutMs > 0) {
        d->connectionTimeoutTimer.start(d->connectionTimeoutMs);
        LOG_DEBUG(QString("Connection timeout set to %1 ms").arg(d->connectionTimeoutMs));
    }

    d->webSocket->open(connectUrl);
}

void WebSocketClient::disconnectFromServer()
{
    Q_D(WebSocketClient);
    if (d->state == QAbstractSocket::UnconnectedState) {
        return;
    }

    LOG_DEBUG("Disconnecting from server");
    d->reconnectTimer.stop();
    d->connectionTimeoutTimer.stop();

    if (!d->sessionId.isEmpty()) {
        d->savedSessionId = d->sessionId;
        LOG_DEBUG(QString("Saved session ID for restore: %1").arg(d->savedSessionId));
    }

    d->sessionId.clear();
}

qint64 WebSocketClient::sendBinaryMessage(const QByteArray& data)
{
    Q_D(WebSocketClient);
    if (d->state != QAbstractSocket::ConnectedState) {
        LOG_WARNING("Cannot send message: not connected to server");
        return -1;
    }

    return d->webSocket->sendBinaryMessage(data);
}

SendResult WebSocketClient::sendMessage(MessagePtr message)
{
    Q_D(WebSocketClient);
    if (d->state != QAbstractSocket::ConnectedState) {
        LOG_WARNING("Cannot send message: not connected to server");
        return SendResult();
    }

    if (!message) {
        LOG_ERROR("Cannot send null message");
        return SendResult();
    }

    quint32 messageId = d->m_nextMessageId.fetchAndAddAcquire(1) + 1;
    message->setMessageId(messageId);

    // 路由消息到对应的处理器进行序列化
    QByteArray data = d->messageRouter->serializeMessage(message);
    if (data.isEmpty()) {
        LOG_ERROR("Failed to serialize message");
        return SendResult();
    }

    qint64 bytesSent = sendBinaryMessage(data);
    if (bytesSent >= 0) {
        LOG_DEBUG(QString("Message sent (messageId: %1)").arg(messageId));
    }
    return SendResult(bytesSent, messageId, message->sessionId(), message->typeId(),
                      bytesSent >= 0 ? QString() : "Failed to send message");
}

QAbstractSocket::SocketState WebSocketClient::state() const
{
    Q_D(const WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    return d->state;
}

QString WebSocketClient::sessionId() const
{
    Q_D(const WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    return d->sessionId;
}

void WebSocketClient::setSessionId(const QString& sessionId)
{
    Q_D(WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    if (d->sessionId != sessionId) {
        d->sessionId = sessionId;
        emit sessionIdChanged(sessionId);
        LOG_DEBUG(QString("Session ID changed to: %1").arg(sessionId));
    }
}

void WebSocketClient::setStateChangedCallback(StateChangedCallback callback)
{
    Q_D(WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    d->stateCallback = callback;
}

void WebSocketClient::setErrorCallback(ErrorCallback callback)
{
    Q_D(WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    d->errorCallback = callback;
}

void WebSocketClient::setAutoReconnect(bool enable, int interval, int maxAttempts)
{
    Q_D(WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    d->autoReconnect = enable;
    d->reconnectInterval = interval;
    d->maxReconnectAttempts = maxAttempts;
    d->reconnectTimer.setInterval(interval);

    if (enable) {
        LOG_DEBUG(QString("Auto reconnect enabled (interval: %1 ms, max attempts: %2)").arg(interval).arg(maxAttempts));
    } else {
        LOG_DEBUG("Auto reconnect disabled");
    }
}

void WebSocketClient::resetReconnectAttempts()
{
    Q_D(WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    d->currentReconnectAttempts = 0;
    LOG_DEBUG("Reconnect attempts reset");
}

int WebSocketClient::currentReconnectAttempts() const
{
    Q_D(const WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    return d->currentReconnectAttempts;
}

int WebSocketClient::maxReconnectAttempts() const
{
    Q_D(const WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    return d->maxReconnectAttempts;
}

void WebSocketClient::setState(QAbstractSocket::SocketState newState)
{
    Q_D(WebSocketClient);
    QMutexLocker locker(&d->m_mutex);
    if (d->state == newState) {
        return;
    }

    QAbstractSocket::SocketState oldState = d->state;
    d->state = newState;

    QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketState>();
    LOG_DEBUG(QString("WebSocket state changed from %1 to %2")
                  .arg(metaEnum.valueToKey(oldState))
                  .arg(metaEnum.valueToKey(newState)));

    emit stateChanged(newState, oldState);
    if (d->stateCallback) {
        d->stateCallback(newState, oldState);
    }
}

void WebSocketClient::onConnected()
{
    Q_D(WebSocketClient);
    d->connectionTimeoutTimer.stop();
    LOG_DEBUG("Connected to server");
    {
        QMutexLocker locker(&d->m_mutex);
        d->sessionId.clear();
        d->currentReconnectAttempts = 0;
    }
    setState(QAbstractSocket::ConnectedState);
}

void WebSocketClient::onDisconnected()
{
    Q_D(WebSocketClient);
    QString reason = d->webSocket->closeReason();
    LOG_DEBUG(QString("Disconnected from server: %1").arg(reason.isEmpty() ? "Connection closed" : reason));

    setState(QAbstractSocket::UnconnectedState);

    {
        QMutexLocker locker(&d->m_mutex);

        if (!d->sessionId.isEmpty()) {
            d->savedSessionId = d->sessionId;
            LOG_DEBUG(QString("Saved session ID for restore: %1").arg(d->savedSessionId));
        }

        d->sessionId.clear();

        if (d->autoReconnect) {
            if (d->maxReconnectAttempts == -1 || d->currentReconnectAttempts < d->maxReconnectAttempts) {
                d->currentReconnectAttempts++;
                LOG_DEBUG(QString("Attempting to reconnect (attempt %1/%2) in %3 ms")
                              .arg(d->currentReconnectAttempts)
                              .arg(d->maxReconnectAttempts == -1 ? "∞" : QString::number(d->maxReconnectAttempts))
                              .arg(d->reconnectInterval));

                emit reconnectAttempted(d->currentReconnectAttempts, d->maxReconnectAttempts);
                d->reconnectTimer.start();
            } else {
                LOG_ERROR("Max reconnect attempts reached");
                emit reconnectFailed();
            }
        }
    }
}

void WebSocketClient::onBinaryMessageReceived(const QByteArray& message)
{
    Q_D(WebSocketClient);
    LOG_DEBUG(QString("Received binary message (%1 bytes)").arg(message.size()));
    d->messageRouter->processMessage(message);
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    Q_D(WebSocketClient);
    QString errorString = d->webSocket->errorString();

    if (error == QAbstractSocket::RemoteHostClosedError) {
        LOG_DEBUG("Received RemoteHostClosedError from server");
    } else {
        LOG_ERROR(QString("WebSocket error: %1 (code: %2)").arg(errorString).arg(static_cast<int>(error)));
    }

    if (d->state != QAbstractSocket::UnconnectedState) {
        setState(QAbstractSocket::UnconnectedState);
    }

    emit errorOccurred(errorString);
    if (d->errorCallback) {
        d->errorCallback(errorString);
    }
}

void WebSocketClient::onReconnectTimerTimeout()
{
    Q_D(WebSocketClient);
    if (d->state == QAbstractSocket::UnconnectedState) {
        LOG_DEBUG("Reconnecting to server");
        connectToServer(d->serverUrl, d->savedSessionId);
    }
}

void WebSocketClient::onConnectionTimeout()
{
    Q_D(WebSocketClient);

    LOG_ERROR(QString("Connection timeout after %1 ms").arg(d->connectionTimeoutMs));

    if (d->webSocket->state() == QAbstractSocket::ConnectedState) {
        d->webSocket->close();
    } else {
        d->webSocket->abort();
    }
    setState(QAbstractSocket::UnconnectedState);

    emit connectionTimeout();
    if (d->connectionTimeoutCallback) {
        d->connectionTimeoutCallback();
    }
}

void WebSocketClient::setConnectionTimeout(int timeoutMs)
{
    Q_D(WebSocketClient);
    d->connectionTimeoutMs = timeoutMs;
    d->connectionTimeoutEnabled = (timeoutMs > 0);
    LOG_DEBUG(QString("Connection timeout set to %1 ms").arg(timeoutMs));
}

int WebSocketClient::connectionTimeout() const
{
    Q_D(const WebSocketClient);
    return d->connectionTimeoutMs;
}

void WebSocketClient::setConnectionTimeoutCallback(std::function<void()> callback)
{
    Q_D(WebSocketClient);
    d->connectionTimeoutCallback = callback;
}

}  // namespace QtWebSocketProtobuf
