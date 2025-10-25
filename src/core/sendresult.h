#ifndef QTWEBSOCKETPROTOBUF_SENDRESULT_H
#define QTWEBSOCKETPROTOBUF_SENDRESULT_H

#include "../qtwebsocketprotobufglobal.h"
#include <QObject>
#include <QScopedPointer>
#include <QString>

namespace QtWebSocketProtobuf
{

class SendResultPrivate;

/**
 * @brief 消息发送结果类
 *
 * 封装消息发送操作的详细结果信息
 */
class QTWEBSOCKETPROTOBUF_EXPORT SendResult
{
    Q_DECLARE_PRIVATE(SendResult)

public:
    SendResult();
    SendResult(qint64 bytesSent, quint32 messageId, quint64 sessionId, int messageType, const QString& error);
    SendResult(const SendResult& other);
    SendResult& operator=(const SendResult& other);
    ~SendResult();

    qint64 bytesSent() const;
    quint32 messageId() const;
    quint64 sessionId() const;
    int messageType() const;
    QString error() const;

    void setBytesSent(qint64 bytes);
    void setMessageId(quint32 id);
    void setSessionId(quint64 sessionId);
    void setMessageType(int type);
    void setError(const QString& error);

    bool isSuccess() const;
    bool isFailed() const;
    bool hasError() const;
    bool hasValidMessageId() const;
    bool hasValidMessageType() const;

private:
    QScopedPointer<SendResultPrivate> d_ptr;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_SENDRESULT_H
