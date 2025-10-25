#ifndef QTWEBSOCKETPROTOBUF_THREADEDWEBSOCKETSERVER_H
#define QTWEBSOCKETPROTOBUF_THREADEDWEBSOCKETSERVER_H

#include "abstractmessageprocessor.h"
#include "broadcastresult.h"
#include "sendresult.h"
#include <QHostAddress>

namespace QtWebSocketProtobuf
{

class ThreadedWebSocketServerPrivate;
class AbstractMessageRouter;

/**
 * @brief 线程化WebSocket服务端
 *
 * 提供线程安全的高性能WebSocket通信服务，所有WebSocket操作在独立线程中执行，
 * 支持消息发送、广播、会话管理和处理器注册等功能
 */
class QTWEBSOCKETPROTOBUF_EXPORT ThreadedWebSocketServer : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ThreadedWebSocketServer)

public:
    explicit ThreadedWebSocketServer(AbstractMessageRouter* router = nullptr, QObject* parent = nullptr);
    ~ThreadedWebSocketServer() override;

    void start(const QHostAddress& address = QHostAddress::Any, quint16 port = 8080);
    void stop();
    bool isRunning() const;
    quint16 serverPort() const;
    QHostAddress serverAddress() const;

    void sendMessage(MessagePtr message);
    void broadcastMessage(MessagePtr message);

    void disconnectSession(const QString& sessionId, const QString& reason = QString());
    void isSessionConnected(const QString& sessionId);
    void sessionCount();

    bool registerProcessor(AbstractMessageProcessorPtr processor, const QList<int>& messageTypeIds);
    void unregisterProcessor(const QList<int>& messageTypeIds);

signals:
    void sessionConnected(const QString& sessionId);
    void sessionDisconnected(const QString& sessionId, const QString& reason);
    void serverError(const QString& error);
    void serverStarted();

    void sendMessageResult(const QtWebSocketProtobuf::SendResult& result);
    void broadcastMessageResult(const QtWebSocketProtobuf::BroadcastResult& result);

    void isSessionConnectedResult(const QString& sessionId, bool connected);
    void sessionCountResult(int count);

private slots:
    void onServerStarted();
    void onServerStopped();

private:
    void cleanup();
    QScopedPointer<ThreadedWebSocketServerPrivate> d_ptr;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_THREADEDWEBSOCKETSERVER_H
