#ifndef TIMEOUTMANAGER_H
#define TIMEOUTMANAGER_H

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QVariant>
#include <QtWebSocketProtobuf/Logger>
#include <functional>

/**
 * @brief 待处理请求结构体
 */
struct PendingRequest
{
    std::function<void(const QVariant&)> callback;
    QTimer* timeoutTimer;
    int timeoutMs = 3000;

    PendingRequest() = default;

    PendingRequest(std::function<void(const QVariant&)> cb, QTimer* timer, int timeout = 3000)
        : callback(std::move(cb)), timeoutTimer(timer), timeoutMs(timeout)
    {
    }
};

/**
 * @brief 超时管理器
 *
 * 使用对象池模式管理超时请求，提供高性能的请求-响应匹配机制。
 */
class TimeoutManager : public QObject
{
    Q_OBJECT

public:
    using RequestId = uint32_t;
    using CallbackType = std::function<void(const QVariant&)>;

    explicit TimeoutManager(int poolSize = 50, QObject* parent = nullptr);
    virtual ~TimeoutManager();

    bool addPendingRequest(RequestId requestId, CallbackType callback, int timeoutMs = 3000);
    bool handleResponse(RequestId requestId, const QVariant& message);
    bool removePendingRequest(RequestId requestId);
    void clearAllPendingRequests();
    int getPendingRequestCount() const;
    QString getPoolStats() const;

signals:
    void requestTimeout(RequestId requestId);
    void requestCompleted(RequestId requestId);

private slots:
    void onRequestTimeout(RequestId requestId);

private:
    QMap<RequestId, PendingRequest> m_pendingRequests;
    QQueue<QTimer*> m_availableTimers;
    QList<QTimer*> m_allTimers;
    int m_poolSize;

    void initializeTimerPool();
    QTimer* acquireTimer();
    void releaseTimer(QTimer* timer);
    void cleanupTimeoutRequest(RequestId requestId);
};

#endif  // TIMEOUTMANAGER_H
