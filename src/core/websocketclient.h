#ifndef QTWEBSOCKETPROTOBUF_WEBSOCKETCLIENT_H
#define QTWEBSOCKETPROTOBUF_WEBSOCKETCLIENT_H

#include "../qtwebsocketprotobufglobal.h"
#include "message.h"
#include "sendresult.h"
#include <QAbstractSocket>
#include <QObject>
#include <QScopedPointer>
#include <QUrl>
#include <functional>

namespace QtWebSocketProtobuf
{

class WebSocketClientPrivate;
class AbstractMessageRouter;

/**
 * @brief WebSocket客户端
 *
 * 处理WebSocket连接和消息收发，提供自动重连和消息处理功能
 */
class QTWEBSOCKETPROTOBUF_EXPORT WebSocketClient : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WebSocketClient)

public:
    using StateChangedCallback =
        std::function<void(QAbstractSocket::SocketState newState, QAbstractSocket::SocketState oldState)>;
    using ErrorCallback = std::function<void(const QString& errorString)>;

    explicit WebSocketClient(AbstractMessageRouter* router, QObject* parent = nullptr);
    virtual ~WebSocketClient() override;

    void connectToServer(const QUrl& url, quint64 sessionId = 0);
    void disconnectFromServer();
    void setStateChangedCallback(StateChangedCallback callback);
    void setErrorCallback(ErrorCallback callback);
    int connectionTimeout() const;
    void setConnectionTimeout(int timeoutMs);
    void setConnectionTimeoutCallback(std::function<void()> callback);
    QAbstractSocket::SocketState state() const;
    inline bool isConnected() const
    {
        return state() == QAbstractSocket::ConnectedState;
    }

    void setAutoReconnect(bool enable, int interval = 5000, int maxAttempts = 5);
    void resetReconnectAttempts();
    int currentReconnectAttempts() const;
    int maxReconnectAttempts() const;

    qint64 sendBinaryMessage(const QByteArray& data);
    SendResult sendMessage(MessagePtr message);

    quint64 sessionId() const;
    void setSessionId(quint64 sessionId);

signals:
    void stateChanged(QAbstractSocket::SocketState newState, QAbstractSocket::SocketState oldState);
    void errorOccurred(const QString& errorString);
    void connectionTimeout();
    void reconnectAttempted(int attempt, int maxAttempts);
    void reconnectFailed();
    void sessionIdChanged(quint64 sessionId);

private slots:
    void onConnected();
    void onDisconnected();
    void onConnectionTimeout();
    void onReconnectTimerTimeout();
    void onBinaryMessageReceived(const QByteArray& message);
    void onError(QAbstractSocket::SocketError error);

private:
    void setState(QAbstractSocket::SocketState newState);

private:
    QScopedPointer<WebSocketClientPrivate> d_ptr;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_WEBSOCKETCLIENT_H
