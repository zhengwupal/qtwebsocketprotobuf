#ifndef QTWEBSOCKETPROTOBUF_BROADCASTRESULT_H
#define QTWEBSOCKETPROTOBUF_BROADCASTRESULT_H

#include "../qtwebsocketprotobufglobal.h"
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

namespace QtWebSocketProtobuf
{

class BroadcastResultPrivate;

/**
 * @brief 广播结果类
 *
 * 封装广播操作的详细结果信息
 */
class QTWEBSOCKETPROTOBUF_EXPORT BroadcastResult
{
    Q_DECLARE_PRIVATE(BroadcastResult)

public:
    BroadcastResult();
    BroadcastResult(qint64 totalBytesSent, int successCount, int totalCount,
                    const QStringList& failedSessions = QStringList(), const QString& error = QString());
    BroadcastResult(const BroadcastResult& other);
    BroadcastResult& operator=(const BroadcastResult& other);
    ~BroadcastResult();

    qint64 totalBytesSent() const;
    int successCount() const;
    int totalCount() const;
    QStringList failedSessions() const;
    QString error() const;

    bool isSuccess() const;
    bool hasFailedSessions() const;
    int failedCount() const;

private:
    QScopedPointer<BroadcastResultPrivate> d_ptr;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_BROADCASTRESULT_H
