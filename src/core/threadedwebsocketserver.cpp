#include "threadedwebsocketserver.h"
#include "../utils/logger.h"
#include "abstractmessageprocessor.h"
#include "abstractmessagerouter.h"
#include "broadcastresult.h"
#include "message.h"
#include "sendresult.h"
#include "threadedwebsocketserver_p.h"
#include "websocketserver.h"
#include <QEventLoop>
#include <QReadLocker>
#include <QScopedPointer>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QWriteLocker>

namespace QtWebSocketProtobuf
{

namespace
{
void registerMetaTypes()
{
    static bool registered = false;
    if (!registered) {
        qRegisterMetaType<SendResult>("SendResult");
        qRegisterMetaType<BroadcastResult>("BroadcastResult");
        qRegisterMetaType<MessagePtr>("MessagePtr");
        registered = true;
    }
}
}  // namespace

WebSocketServerWorker::WebSocketServerWorker(AbstractMessageRouter* router, QObject* parent)
    : QObject(parent), m_server(nullptr), m_router(router)
{
}

WebSocketServerWorker::~WebSocketServerWorker()
{
    cleanup();
}

void WebSocketServerWorker::setServerParameters(const QHostAddress& address, quint16 port)
{
    m_address = address;
    m_port = port;
}

void WebSocketServerWorker::sendMessage(MessagePtr message)
{
    if (!m_server || !message) {
        SendResult result(-1, 0, message ? message->sessionId() : 0, message ? message->messageType() : 0,
                          "Server not available or null message");
        emit sendMessageResult(result);
        return;
    }

    qint64 bytesSent = m_server->sendMessage(message);
    SendResult result(bytesSent, message->messageId(), message->sessionId(), message->messageType(),
                      bytesSent > 0 ? QString() : "Failed to send message");
    emit sendMessageResult(result);
}

void WebSocketServerWorker::broadcastMessage(MessagePtr message)
{
    if (!m_server || !message) {
        BroadcastResult result(-1, 0, 0, QStringList(), "Server not available or null message");
        emit broadcastMessageResult(result);
        return;
    }

    QByteArray data = m_server->messageRouter()->serializeMessage(message);
    if (data.isEmpty()) {
        BroadcastResult result(-1, 0, 0, QStringList(), "Failed to serialize broadcast message");
        emit broadcastMessageResult(result);
        return;
    }

    BroadcastResult result = m_server->broadcastBinaryMessage(data);
    emit broadcastMessageResult(result);
}

void WebSocketServerWorker::disconnectSession(quint64 sessionId, const QString& reason)
{
    if (m_server) {
        m_server->disconnectSession(sessionId, reason);
    }
}

void WebSocketServerWorker::isSessionConnected(quint64 sessionId)
{
    bool connected = false;
    if (m_server) {
        connected = m_server->isSessionConnected(sessionId);
    }
    emit isSessionConnectedResult(sessionId, connected);
}

void WebSocketServerWorker::sessionCount()
{
    int count = 0;
    if (m_server) {
        count = m_server->sessionCount();
    }
    emit sessionCountResult(count);
}

bool WebSocketServerWorker::registerProcessor(AbstractMessageProcessorPtr processor, const QList<int>& messageTypeIds)
{
    if (!m_server || !processor) {
        LOG_ERROR("Failed to register processor: WebSocketServer is not available or processor is null");
        return false;
    }

    m_server->messageRouter()->registerProcessor(processor, messageTypeIds);
    return true;
}

void WebSocketServerWorker::unregisterProcessor(const QList<int>& messageTypeIds)
{
    if (m_server) {
        m_server->messageRouter()->unregisterProcessor(messageTypeIds);
    }
}

void WebSocketServerWorker::startServer()
{
    if (m_server) {
        return;
    }

    if (!m_router) {
        LOG_ERROR("Cannot start server: router is null");
        emit serverError("Router is null");
        return;
    }

    m_server = new WebSocketServer(m_router, this);

    connect(m_server, &WebSocketServer::serverError, this, &WebSocketServerWorker::serverError);
    connect(m_server, &WebSocketServer::sessionConnected, this,
            [this](quint64 sessionId) { emit sessionConnected(sessionId); });
    connect(m_server, &WebSocketServer::sessionDisconnected, this,
            [this](quint64 sessionId, const QString& reason) { emit sessionDisconnected(sessionId, reason); });

    if (!m_server->start(m_address, m_port)) {
        LOG_ERROR("Failed to start server");
        cleanup();
        emit serverError("Failed to start server");
        return;
    }

    emit serverStarted();
}

void WebSocketServerWorker::cleanup()
{
    if (m_server) {
        m_server->stop();
        delete m_server;
        m_server = nullptr;
        emit serverStopped();
    }
}

ThreadedWebSocketServer::ThreadedWebSocketServer(AbstractMessageRouter* router, QObject* parent)
    : QObject(parent), d_ptr(new ThreadedWebSocketServerPrivate(this))
{
    registerMetaTypes();
    Q_D(ThreadedWebSocketServer);
    d->router = router;
}

ThreadedWebSocketServer::~ThreadedWebSocketServer()
{
    Q_D(ThreadedWebSocketServer);
    stop();
}

void ThreadedWebSocketServer::start(const QHostAddress& address, quint16 port)
{
    Q_D(ThreadedWebSocketServer);

    if (d->running.loadAcquire()) {
        LOG_WARNING("Server is already running");
        return;
    }

    if (!d->router) {
        LOG_ERROR("Cannot start server: router is null");
        return;
    }

    d->address = address;
    d->port = port;

    d->workerThread = new QThread(this);
    if (!d->workerThread) {
        LOG_ERROR("Failed to create worker thread");
        return;
    }

    d->serverWorker = new WebSocketServerWorker(d->router);
    if (!d->serverWorker) {
        LOG_ERROR("Failed to create server worker");
        delete d->workerThread;
        d->workerThread = nullptr;
        return;
    }

    d->serverWorker->setServerParameters(address, port);
    d->serverWorker->moveToThread(d->workerThread);
    d->router->moveToThread(d->workerThread);

    connect(d->workerThread, &QThread::started, d->serverWorker, &WebSocketServerWorker::startServer,
            Qt::QueuedConnection);

    connect(d->serverWorker, &WebSocketServerWorker::sessionConnected, this, &ThreadedWebSocketServer::sessionConnected,
            Qt::QueuedConnection);
    connect(d->serverWorker, &WebSocketServerWorker::sessionDisconnected, this,
            &ThreadedWebSocketServer::sessionDisconnected, Qt::QueuedConnection);
    connect(d->serverWorker, &WebSocketServerWorker::serverError, this, &ThreadedWebSocketServer::serverError,
            Qt::QueuedConnection);
    connect(d->serverWorker, &WebSocketServerWorker::serverStarted, this, &ThreadedWebSocketServer::onServerStarted,
            Qt::QueuedConnection);
    connect(d->serverWorker, &WebSocketServerWorker::serverStopped, this, &ThreadedWebSocketServer::onServerStopped,
            Qt::QueuedConnection);

    // 连接消息发送结果信号
    connect(d->serverWorker, &WebSocketServerWorker::sendMessageResult, this,
            &ThreadedWebSocketServer::sendMessageResult, Qt::QueuedConnection);
    connect(d->serverWorker, &WebSocketServerWorker::broadcastMessageResult, this,
            &ThreadedWebSocketServer::broadcastMessageResult, Qt::QueuedConnection);

    // 连接查询结果信号
    connect(d->serverWorker, &WebSocketServerWorker::isSessionConnectedResult, this,
            &ThreadedWebSocketServer::isSessionConnectedResult, Qt::QueuedConnection);
    connect(d->serverWorker, &WebSocketServerWorker::sessionCountResult, this,
            &ThreadedWebSocketServer::sessionCountResult, Qt::QueuedConnection);

    d->workerThread->start();

    if (!d->workerThread->isRunning()) {
        LOG_ERROR("Failed to start worker thread");
        cleanup();
        return;
    }
}

void ThreadedWebSocketServer::stop()
{
    Q_D(ThreadedWebSocketServer);
    if (!d->running.loadAcquire()) {
        return;
    }
    cleanup();
}

bool ThreadedWebSocketServer::isRunning() const
{
    Q_D(const ThreadedWebSocketServer);
    return d->running.loadAcquire();
}

quint16 ThreadedWebSocketServer::serverPort() const
{
    Q_D(const ThreadedWebSocketServer);
    return d->port;
}

QHostAddress ThreadedWebSocketServer::serverAddress() const
{
    Q_D(const ThreadedWebSocketServer);
    return d->address;
}

void ThreadedWebSocketServer::broadcastMessage(MessagePtr message)
{
    Q_D(ThreadedWebSocketServer);
    if (!d->running.loadAcquire() || !d->serverWorker) {
        return;
    }

    QMetaObject::invokeMethod(d->serverWorker, "broadcastMessage", Qt::QueuedConnection, Q_ARG(MessagePtr, message));
}

void ThreadedWebSocketServer::sendMessage(MessagePtr message)
{
    Q_D(ThreadedWebSocketServer);
    if (!d->running.loadAcquire() || !d->serverWorker) {
        return;
    }

    QMetaObject::invokeMethod(d->serverWorker, "sendMessage", Qt::QueuedConnection, Q_ARG(MessagePtr, message));
}

void ThreadedWebSocketServer::disconnectSession(quint64 sessionId, const QString& reason)
{
    Q_D(ThreadedWebSocketServer);
    if (!d->running.loadAcquire() || !d->serverWorker) {
        return;
    }

    QMetaObject::invokeMethod(d->serverWorker, "disconnectSession", Qt::QueuedConnection, Q_ARG(quint64, sessionId),
                              Q_ARG(QString, reason));
}

void ThreadedWebSocketServer::isSessionConnected(quint64 sessionId)
{
    Q_D(ThreadedWebSocketServer);
    if (!d->running.loadAcquire() || !d->serverWorker) {
        return;
    }

    QMetaObject::invokeMethod(d->serverWorker, "isSessionConnected", Qt::QueuedConnection, Q_ARG(quint64, sessionId));
}

void ThreadedWebSocketServer::sessionCount()
{
    Q_D(ThreadedWebSocketServer);
    if (!d->running.loadAcquire() || !d->serverWorker) {
        return;
    }

    QMetaObject::invokeMethod(d->serverWorker, "sessionCount", Qt::QueuedConnection);
}

bool ThreadedWebSocketServer::registerProcessor(AbstractMessageProcessorPtr processor, const QList<int>& messageTypeIds)
{
    Q_D(ThreadedWebSocketServer);

    if (!processor) {
        LOG_ERROR("Cannot register null processor");
        return false;
    }

    if (messageTypeIds.isEmpty()) {
        LOG_ERROR("Cannot register processor with empty message type list");
        return false;
    }

    if (!d->running.loadAcquire() || !d->serverWorker) {
        ThreadedWebSocketServerPrivate::PendingProcessor pending;
        pending.processor = processor;
        pending.messageTypeIds = messageTypeIds;

        QWriteLocker locker(&d->pendingProcessorsLock);
        d->pendingProcessors.append(pending);
        return true;
    }

    QMetaObject::invokeMethod(d->serverWorker, "registerProcessor", Qt::QueuedConnection,
                              Q_ARG(AbstractMessageProcessorPtr, processor), Q_ARG(QList<int>, messageTypeIds));
    return true;
}

void ThreadedWebSocketServer::unregisterProcessor(const QList<int>& messageTypeIds)
{
    Q_D(ThreadedWebSocketServer);

    if (messageTypeIds.isEmpty()) {
        return;
    }

    if (!d->running.loadAcquire() || !d->serverWorker) {
        QWriteLocker locker(&d->pendingProcessorsLock);

        QList<ThreadedWebSocketServerPrivate::PendingProcessor> remainingProcessors;
        for (const auto& pending : qAsConst(d->pendingProcessors)) {
            QList<int> remainingTypes;
            for (int typeId : pending.messageTypeIds) {
                if (!messageTypeIds.contains(typeId)) {
                    remainingTypes.append(typeId);
                }
            }

            if (!remainingTypes.isEmpty()) {
                ThreadedWebSocketServerPrivate::PendingProcessor newPending;
                newPending.processor = pending.processor;
                newPending.messageTypeIds = remainingTypes;
                remainingProcessors.append(newPending);
            }
        }

        d->pendingProcessors = remainingProcessors;
        return;
    }

    QMetaObject::invokeMethod(d->serverWorker, "unregisterProcessor", Qt::QueuedConnection,
                              Q_ARG(QList<int>, messageTypeIds));
}

void ThreadedWebSocketServer::onServerStarted()
{
    Q_D(ThreadedWebSocketServer);

    d->running.storeRelease(true);
    LOG_DEBUG("Server is now running");

    emit serverStarted();

    QList<ThreadedWebSocketServerPrivate::PendingProcessor> processorsCopy;
    {
        QReadLocker locker(&d->pendingProcessorsLock);
        if (d->pendingProcessors.isEmpty()) {
            return;
        }
        processorsCopy = d->pendingProcessors;
    }

    for (const auto& pending : qAsConst(processorsCopy)) {
        bool success = false;
        QMetaObject::invokeMethod(d->serverWorker, "registerProcessor", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, success), Q_ARG(AbstractMessageProcessorPtr, pending.processor),
                                  Q_ARG(QList<int>, pending.messageTypeIds));

        if (!success) {
            QStringList typeIds;
            for (int typeId : pending.messageTypeIds) {
                typeIds.append(QString::number(typeId));
            }
            LOG_ERROR(QString("Failed to register pending processor for message types: %1").arg(typeIds.join(", ")));
        }
    }

    {
        QWriteLocker locker(&d->pendingProcessorsLock);
        d->pendingProcessors.clear();
    }
}

void ThreadedWebSocketServer::onServerStopped() {}

void ThreadedWebSocketServer::cleanup()
{
    Q_D(ThreadedWebSocketServer);

    if (d->serverWorker) {
        if (d->workerThread && d->workerThread->isRunning()) {
            connect(
                d->serverWorker, &WebSocketServerWorker::serverStopped, this,
                [this, d]() {
                    delete d->serverWorker;
                    d->serverWorker = nullptr;

                    if (d->workerThread) {
                        d->workerThread->quit();
                        d->workerThread->wait();
                        delete d->workerThread;
                        d->workerThread = nullptr;
                    }

                    d->running.storeRelease(false);
                },
                Qt::QueuedConnection);

            QMetaObject::invokeMethod(d->serverWorker, &WebSocketServerWorker::cleanup, Qt::QueuedConnection);
        } else {
            delete d->serverWorker;
            d->serverWorker = nullptr;

            if (d->workerThread) {
                delete d->workerThread;
                d->workerThread = nullptr;
            }

            d->running.storeRelease(false);
        }
    }
}

}  // namespace QtWebSocketProtobuf
