#ifndef QTWEBSOCKETPROTOBUF_MESSAGE_H
#define QTWEBSOCKETPROTOBUF_MESSAGE_H

#include "../qtwebsocketprotobufglobal.h"
#include <QScopedPointer>
#include <QSharedPointer>

namespace QtWebSocketProtobuf
{

class MessagePrivate;

/**
 * @brief 消息基类接口
 *
 * 所有消息类型的基类，提供统一的接口获取消息类型和序列化功能
 */
class QTWEBSOCKETPROTOBUF_EXPORT Message
{
public:
    virtual ~Message();

    virtual quint16 messageType() const = 0;

    virtual QByteArray serialize() const = 0;

    quint64 sessionId() const;
    void setSessionId(quint64 sessionId);

    quint32 messageId() const;
    void setMessageId(quint32 messageId);

protected:
    Message();
    QScopedPointer<MessagePrivate> d_ptr;
    Q_DECLARE_PRIVATE(Message)
};

using MessagePtr = QSharedPointer<Message>;

/**
 * @brief 获取消息的强类型引用
 *
 * @tparam T 消息类型
 * @param message 消息智能指针
 * @return const T& 消息对象的引用，如果类型不匹配则抛出异常
 * @throws std::bad_cast 如果类型转换失败
 */
template <typename T> const T& getMessage(MessagePtr message)
{
    auto typedMessage = qSharedPointerDynamicCast<T>(message);
    if (!typedMessage) {
        throw std::bad_cast();
    }
    return *typedMessage;
}

/**
 * @brief 安全获取消息的强类型引用
 *
 * @tparam T 消息类型
 * @param message 消息智能指针
 * @param ok 如果非空，设置为是否成功获取
 * @return T* 消息对象的指针，如果类型不匹配则返回nullptr
 */
template <typename T> T* getMessageSafe(MessagePtr message, bool* ok = nullptr)
{
    auto typedMessage = qSharedPointerDynamicCast<T>(message);
    if (ok)
        *ok = (typedMessage != nullptr);
    return typedMessage.data();
}

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_MESSAGE_H
