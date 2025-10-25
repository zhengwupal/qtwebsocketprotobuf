#ifndef QTWEBSOCKETPROTOBUF_THREADEDWEBSOCKETSERVER_P_H
#define QTWEBSOCKETPROTOBUF_THREADEDWEBSOCKETSERVER_P_H

#include "abstractmessageprocessor.h"
#include "abstractmessagerouter.h"
#include "broadcastresult.h"
#include "message.h"
#include "sendresult.h"
#include "threadedwebsocketserver.h"
#include <QHash>
#include <QHostAddress>
#include <QObject>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QThread>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QWriteLocker>
#include <functional>

namespace QtWebSocketProtobuf
{

class WebSocketServerWorker;
class WebSocketServer;

class ThreadedWebSocketServerPrivate
{
    Q_DECLARE_PUBLIC(ThreadedWebSocketServer)

public:
    ThreadedWebSocketServerPrivate(ThreadedWebSocketServer* q) : q_ptr(q) {}

    ThreadedWebSocketServer* const q_ptr;

    struct PendingProcessor
    {
        AbstractMessageProcessorPtr processor;
        QList<int> messageTypeIds;
    };
    QList<PendingProcessor> pendingProcessors;
    QReadWriteLock pendingProcessorsLock;

    QThread* workerThread = nullptr;
    WebSocketServerWorker* serverWorker = nullptr;
    AbstractMessageRouter* router = nullptr;
    QHostAddress address;
    quint16 port = 0;
    QAtomicInt running;
};

class QTWEBSOCKETPROTOBUF_EXPORT WebSocketServerWorker : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketServerWorker(AbstractMessageRouter* router, QObject* parent = nullptr);
    ~WebSocketServerWorker();

    void setServerParameters(const QHostAddress& address, quint16 port);

public slots:
    void startServer();
    void cleanup();

    void sendMessage(MessagePtr message);
    void broadcastMessage(MessagePtr message);

    void disconnectSession(quint64 sessionId, const QString& reason = QString());
    void isSessionConnected(quint64 sessionId);
    void sessionCount();

    bool registerProcessor(AbstractMessageProcessorPtr processor, const QList<int>& messageTypeIds);
    void unregisterProcessor(const QList<int>& messageTypeIds);

signals:
    void serverStarted();
    void serverStopped();
    void sessionConnected(quint64 sessionId);
    void sessionDisconnected(quint64 sessionId, const QString& reason);
    void serverError(const QString& error);

    void sendMessageResult(const SendResult& result);
    void broadcastMessageResult(const BroadcastResult& result);

    void isSessionConnectedResult(quint64 sessionId, bool connected);
    void sessionCountResult(int count);

private:
    WebSocketServer* m_server;
    AbstractMessageRouter* m_router;
    QHostAddress m_address;
    quint16 m_port;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_THREADEDWEBSOCKETSERVER_P_H
