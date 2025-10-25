#include "sendresult.h"
#include "sendresult_p.h"

namespace QtWebSocketProtobuf
{

SendResultPrivate::SendResultPrivate() : bytesSent(-1), messageId(0), messageType(0) {}

SendResultPrivate::SendResultPrivate(qint64 bytesSent, quint32 messageId, const QString& sessionId, int messageType,
                                     const QString& error)
    : bytesSent(bytesSent), messageId(messageId), sessionId(sessionId), messageType(messageType), error(error)
{
}

SendResultPrivate::SendResultPrivate(const SendResultPrivate& other)
    : bytesSent(other.bytesSent), messageId(other.messageId), sessionId(other.sessionId),
      messageType(other.messageType), error(other.error)
{
}

SendResultPrivate& SendResultPrivate::operator=(const SendResultPrivate& other)
{
    if (this != &other) {
        bytesSent = other.bytesSent;
        messageId = other.messageId;
        sessionId = other.sessionId;
        messageType = other.messageType;
        error = other.error;
    }
    return *this;
}

SendResult::SendResult() : d_ptr(new SendResultPrivate()) {}

SendResult::SendResult(qint64 bytesSent, quint32 messageId, const QString& sessionId, int messageType,
                       const QString& error)
    : d_ptr(new SendResultPrivate(bytesSent, messageId, sessionId, messageType, error))
{
}

SendResult::SendResult(const SendResult& other) : d_ptr(new SendResultPrivate(*other.d_ptr)) {}

SendResult& SendResult::operator=(const SendResult& other)
{
    if (this != &other) {
        *d_ptr = *other.d_ptr;
    }
    return *this;
}

SendResult::~SendResult() {}

qint64 SendResult::bytesSent() const
{
    return d_ptr->bytesSent;
}

quint32 SendResult::messageId() const
{
    return d_ptr->messageId;
}

QString SendResult::sessionId() const
{
    return d_ptr->sessionId;
}

int SendResult::messageType() const
{
    return d_ptr->messageType;
}

QString SendResult::error() const
{
    return d_ptr->error;
}

void SendResult::setBytesSent(qint64 bytes)
{
    d_ptr->bytesSent = bytes;
}

void SendResult::setMessageId(quint32 id)
{
    d_ptr->messageId = id;
}

void SendResult::setSessionId(const QString& sessionId)
{
    d_ptr->sessionId = sessionId;
}

void SendResult::setMessageType(int type)
{
    d_ptr->messageType = type;
}

void SendResult::setError(const QString& error)
{
    d_ptr->error = error;
}

bool SendResult::isSuccess() const
{
    return d_ptr->bytesSent >= 0 && d_ptr->error.isEmpty();
}

bool SendResult::isFailed() const
{
    return d_ptr->bytesSent < 0 || !d_ptr->error.isEmpty();
}

bool SendResult::hasError() const
{
    return !d_ptr->error.isEmpty();
}

bool SendResult::hasValidMessageId() const
{
    return d_ptr->messageId > 0;
}

bool SendResult::hasValidMessageType() const
{
    return d_ptr->messageType >= 0;
}

}  // namespace QtWebSocketProtobuf
