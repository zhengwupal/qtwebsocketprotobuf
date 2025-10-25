#ifndef QTWEBSOCKETPROTOBUF_MESSAGE_P_H
#define QTWEBSOCKETPROTOBUF_MESSAGE_P_H

#include <QString>

namespace QtWebSocketProtobuf
{

class MessagePrivate
{
public:
    QString sessionId;
    quint32 messageId = 0;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_MESSAGE_P_H
