#ifndef QTWEBSOCKETPROTOBUF_WEBSOCKETSERVER_H
#define QTWEBSOCKETPROTOBUF_WEBSOCKETSERVER_H

#include "../qtwebsocketprotobufglobal.h"
#include "broadcastresult.h"
#include "message.h"
#include <QHostAddress>
#include <QObject>
#include <QScopedPointer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <functional>

namespace QtWebSocketProtobuf
{

class WebSocketServerPrivate;
class AbstractMessageRouter;
class SessionManager;

/**
 * @brief WebSocket服务端
 *
 * 处理WebSocket连接和消息分发，提供会话管理和消息处理功能
 */
class QTWEBSOCKETPROTOBUF_EXPORT WebSocketServer : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WebSocketServer)

public:
    using SessionConnectedCallback = std::function<void(quint64 sessionId)>;
    using SessionDisconnectedCallback = std::function<void(quint64 sessionId, const QString& reason)>;
    using ServerErrorCallback = std::function<void(const QString& errorString)>;

    explicit WebSocketServer(AbstractMessageRouter* router, QObject* parent = nullptr);
    virtual ~WebSocketServer() override;

    bool start(const QHostAddress& address, quint16 port);
    void stop();
    bool isRunning() const;
    quint16 serverPort() const;
    QHostAddress serverAddress() const;

    qint64 sendBinaryMessage(quint64 sessionId, const QByteArray& data);
    qint64 sendMessage(MessagePtr message);

    BroadcastResult broadcastBinaryMessage(const QByteArray& data);
    void broadcastMessage(MessagePtr message);

    int sessionCount() const;
    bool isSessionConnected(quint64 sessionId) const;
    void disconnectSession(quint64 sessionId, const QString& reason = QString());
    void setSessionConnectedCallback(SessionConnectedCallback callback);
    void setSessionDisconnectedCallback(SessionDisconnectedCallback callback);
    void setServerErrorCallback(ServerErrorCallback callback);

    AbstractMessageRouter* messageRouter();

signals:
    void sessionConnected(quint64 sessionId);
    void sessionDisconnected(quint64 sessionId, const QString& reason);
    void serverError(const QString& errorString);

private slots:
    void onNewConnection();
    void onSocketDisconnected();
    void onBinaryMessageReceived(const QByteArray& message);
    void onSocketError(QAbstractSocket::SocketError error);
    void onServerError(QWebSocketProtocol::CloseCode closeCode);

private:
    QScopedPointer<WebSocketServerPrivate> d_ptr;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_WEBSOCKETSERVER_H
