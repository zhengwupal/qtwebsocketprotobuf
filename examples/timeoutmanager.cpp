#include "timeoutmanager.h"

TimeoutManager::TimeoutManager(int poolSize, QObject* parent) : QObject(parent), m_poolSize(poolSize)
{
    initializeTimerPool();
}

TimeoutManager::~TimeoutManager()
{
    clearAllPendingRequests();

    for (QTimer* timer : m_allTimers) {
        timer->stop();
        timer->deleteLater();
    }
    m_allTimers.clear();
    m_availableTimers.clear();
}

void TimeoutManager::initializeTimerPool()
{
    for (int i = 0; i < m_poolSize; ++i) {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        m_availableTimers.enqueue(timer);
        m_allTimers.append(timer);
    }

    LOG_DEBUG(QString("Initialized timer pool with %1 timers").arg(m_poolSize));
}

QTimer* TimeoutManager::acquireTimer()
{
    if (m_availableTimers.isEmpty()) {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        m_allTimers.append(timer);
        LOG_WARNING(QString("Pool exhausted, created new timer. Total timers: %1").arg(m_allTimers.size()));
        return timer;
    }

    return m_availableTimers.dequeue();
}

void TimeoutManager::releaseTimer(QTimer* timer)
{
    if (!timer)
        return;

    timer->stop();
    timer->disconnect();

    if (m_availableTimers.contains(timer)) {
        return;
    }

    m_availableTimers.enqueue(timer);
}

bool TimeoutManager::addPendingRequest(RequestId requestId, CallbackType callback, int timeoutMs)
{
    if (m_pendingRequests.contains(requestId)) {
        LOG_WARNING(QString("Request ID %1 already exists").arg(requestId));
        return false;
    }

    QTimer* timer = acquireTimer();
    if (!timer) {
        LOG_ERROR(QString("Failed to acquire timer for request %1").arg(requestId));
        return false;
    }

    connect(timer, &QTimer::timeout, [this, requestId]() { onRequestTimeout(requestId); });

    PendingRequest pendingReq(std::move(callback), timer, timeoutMs);
    m_pendingRequests[requestId] = std::move(pendingReq);

    timer->setInterval(timeoutMs);
    timer->start();

    LOG_DEBUG(QString("Added pending request %1 with timeout %2 ms").arg(requestId).arg(timeoutMs));
    return true;
}

bool TimeoutManager::handleResponse(RequestId requestId, const QVariant& message)
{
    auto it = m_pendingRequests.find(requestId);
    if (it == m_pendingRequests.end()) {
        LOG_WARNING(QString("Received response for unknown request ID %1").arg(requestId));
        return false;
    }

    releaseTimer(it.value().timeoutTimer);

    if (it.value().callback) {
        it.value().callback(message);
    }

    m_pendingRequests.erase(it);

    emit requestCompleted(requestId);
    LOG_DEBUG(QString("Completed request %1").arg(requestId));
    return true;
}

bool TimeoutManager::removePendingRequest(RequestId requestId)
{
    auto it = m_pendingRequests.find(requestId);
    if (it == m_pendingRequests.end()) {
        return false;
    }

    releaseTimer(it.value().timeoutTimer);

    m_pendingRequests.erase(it);
    LOG_DEBUG(QString("Removed pending request %1").arg(requestId));
    return true;
}

void TimeoutManager::clearAllPendingRequests()
{
    int count = m_pendingRequests.size();

    for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
        releaseTimer(it.value().timeoutTimer);
    }

    m_pendingRequests.clear();
    if (count > 0) {
        LOG_DEBUG(QString("Cleared %1 pending requests").arg(count));
    }
}

int TimeoutManager::getPendingRequestCount() const
{
    return m_pendingRequests.size();
}

QString TimeoutManager::getPoolStats() const
{
    return QString("Pool: %1/%2 available, %3 pending requests")
        .arg(m_availableTimers.size())
        .arg(m_allTimers.size())
        .arg(m_pendingRequests.size());
}

void TimeoutManager::onRequestTimeout(RequestId requestId)
{
    auto it = m_pendingRequests.find(requestId);
    if (it == m_pendingRequests.end()) {
        return;
    }

    LOG_WARNING(QString("Request %1 timed out").arg(requestId));

    releaseTimer(it.value().timeoutTimer);

    m_pendingRequests.erase(it);

    emit requestTimeout(requestId);
}

void TimeoutManager::cleanupTimeoutRequest(RequestId requestId)
{
    auto it = m_pendingRequests.find(requestId);
    if (it != m_pendingRequests.end()) {
        releaseTimer(it.value().timeoutTimer);
        m_pendingRequests.erase(it);
    }
}
