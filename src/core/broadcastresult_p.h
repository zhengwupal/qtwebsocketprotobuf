#ifndef QTWEBSOCKETPROTOBUF_BROADCASTRESULT_P_H
#define QTWEBSOCKETPROTOBUF_BROADCASTRESULT_P_H

#include <QString>
#include <QStringList>

namespace QtWebSocketProtobuf
{

class BroadcastResultPrivate
{
public:
    BroadcastResultPrivate();
    BroadcastResultPrivate(qint64 totalBytesSent, int successCount, int totalCount, const QStringList& failedSessions,
                           const QString& error);
    BroadcastResultPrivate(const BroadcastResultPrivate& other);
    BroadcastResultPrivate& operator=(const BroadcastResultPrivate& other);

    qint64 totalBytesSent;
    int successCount;
    int totalCount;
    QStringList failedSessions;
    QString error;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_BROADCASTRESULT_P_H
