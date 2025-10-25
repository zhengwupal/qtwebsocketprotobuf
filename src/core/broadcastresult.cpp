#include "broadcastresult.h"
#include "broadcastresult_p.h"

namespace QtWebSocketProtobuf
{

BroadcastResultPrivate::BroadcastResultPrivate() : totalBytesSent(-1), successCount(0), totalCount(0) {}

BroadcastResultPrivate::BroadcastResultPrivate(qint64 totalBytesSent, int successCount, int totalCount,
                                               const QStringList& failedSessions, const QString& error)
    : totalBytesSent(totalBytesSent), successCount(successCount), totalCount(totalCount),
      failedSessions(failedSessions), error(error)
{
}

BroadcastResultPrivate::BroadcastResultPrivate(const BroadcastResultPrivate& other)
    : totalBytesSent(other.totalBytesSent), successCount(other.successCount), totalCount(other.totalCount),
      failedSessions(other.failedSessions), error(other.error)
{
}

BroadcastResultPrivate& BroadcastResultPrivate::operator=(const BroadcastResultPrivate& other)
{
    if (this != &other) {
        totalBytesSent = other.totalBytesSent;
        successCount = other.successCount;
        totalCount = other.totalCount;
        failedSessions = other.failedSessions;
        error = other.error;
    }
    return *this;
}

BroadcastResult::BroadcastResult() : d_ptr(new BroadcastResultPrivate()) {}

BroadcastResult::BroadcastResult(qint64 totalBytesSent, int successCount, int totalCount,
                                 const QStringList& failedSessions, const QString& error)
    : d_ptr(new BroadcastResultPrivate(totalBytesSent, successCount, totalCount, failedSessions, error))
{
}

BroadcastResult::BroadcastResult(const BroadcastResult& other) : d_ptr(new BroadcastResultPrivate(*other.d_ptr)) {}

BroadcastResult& BroadcastResult::operator=(const BroadcastResult& other)
{
    if (this != &other) {
        *d_ptr = *other.d_ptr;
    }
    return *this;
}

BroadcastResult::~BroadcastResult() {}

qint64 BroadcastResult::totalBytesSent() const
{
    return d_ptr->totalBytesSent;
}

int BroadcastResult::successCount() const
{
    return d_ptr->successCount;
}

int BroadcastResult::totalCount() const
{
    return d_ptr->totalCount;
}

QStringList BroadcastResult::failedSessions() const
{
    return d_ptr->failedSessions;
}

QString BroadcastResult::error() const
{
    return d_ptr->error;
}

bool BroadcastResult::isSuccess() const
{
    return d_ptr->successCount > 0 && d_ptr->error.isEmpty();
}

bool BroadcastResult::hasFailedSessions() const
{
    return !d_ptr->failedSessions.isEmpty();
}

int BroadcastResult::failedCount() const
{
    return d_ptr->failedSessions.size();
}

}  // namespace QtWebSocketProtobuf
