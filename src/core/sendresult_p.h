#ifndef QTWEBSOCKETPROTOBUF_SENDRESULT_P_H
#define QTWEBSOCKETPROTOBUF_SENDRESULT_P_H

#include <QString>

namespace QtWebSocketProtobuf
{

class SendResultPrivate
{
public:
    SendResultPrivate();
    SendResultPrivate(qint64 bytesSent, quint32 messageId, quint64 sessionId, int messageType, const QString& error);
    SendResultPrivate(const SendResultPrivate& other);
    SendResultPrivate& operator=(const SendResultPrivate& other);

    qint64 bytesSent;
    quint32 messageId;
    quint64 sessionId;
    int messageType;
    QString error;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_SENDRESULT_P_H
