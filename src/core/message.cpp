#include "message.h"
#include "core/message_p.h"
#include <QString>

namespace QtWebSocketProtobuf
{

Message::Message() : d_ptr(new MessagePrivate) {}
Message::~Message() = default;

quint64 Message::sessionId() const
{
    Q_D(const Message);
    return d->sessionId;
}
void Message::setSessionId(quint64 sessionId)
{
    Q_D(Message);
    d->sessionId = sessionId;
}

quint32 Message::messageId() const
{
    Q_D(const Message);
    return d->messageId;
}

void Message::setMessageId(quint32 messageId)
{
    Q_D(Message);
    d->messageId = messageId;
}

}  // namespace QtWebSocketProtobuf
