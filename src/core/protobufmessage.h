#ifndef QTWEBSOCKETPROTOBUF_PROTOBUFMESSAGE_H
#define QTWEBSOCKETPROTOBUF_PROTOBUFMESSAGE_H

#include "message.h"
#include "messageserializer.h"
#include "packetserializer.h"

namespace QtWebSocketProtobuf
{

/**
 * @brief Protobuf消息的模板封装类
 *
 * 用于将任意Protobuf消息对象适配为Message抽象基类
 */
template <typename T> class ProtobufMessage : public Message
{
public:
    ProtobufMessage(const T& message, quint16 messageType) : m_message(message), m_messageType(messageType) {}

    quint16 messageType() const override
    {
        return m_messageType;
    }

    QByteArray serialize() const override
    {
        QByteArray payload = MessageSerializer::serializeMessage(m_message);

        return PacketSerializer::serializePacket(static_cast<quint32>(payload.size()), m_messageType, messageId(),
                                                 sessionId(), payload);
    }

    const T& getMessage() const
    {
        return m_message;
    }

    T& getMessage()
    {
        return m_message;
    }

private:
    T m_message;
    quint16 m_messageType;
};

/**
 * @brief 创建消息的辅助函数
 *
 * @tparam T Protobuf消息类型
 * @param message Protobuf消息对象
 * @param messageType 消息类型ID
 * @param sessionId 会话ID（可选，默认为0）
 * @return MessagePtr 封装了消息对象的智能指针
 */
template <typename T> MessagePtr createMessage(const T& message, quint16 messageType, quint64 sessionId = 0)
{
    auto msgPtr = MessagePtr(new ProtobufMessage<T>(message, messageType));
    if (sessionId != 0) {
        msgPtr->setSessionId(sessionId);
    }
    return msgPtr;
}

/**
 * @brief 从消息智能指针获取原始Protobuf消息
 *
 * @tparam T 期望的Protobuf消息类型
 * @param message 消息智能指针
 * @return const T& Protobuf消息对象的常量引用
 * @throws std::bad_cast 如果消息类型不匹配
 */
template <typename T> const T& getProtobufMessage(MessagePtr message)
{
    auto typedMessage = qSharedPointerDynamicCast<ProtobufMessage<T>>(message);
    if (!typedMessage) {
        throw std::bad_cast();
    }
    return typedMessage->getMessage();
}

/**
 * @brief 安全获取原始Protobuf消息
 *
 * @tparam T 期望的Protobuf消息类型
 * @param message 消息智能指针
 * @param ok 如果非空，设置为是否成功获取
 * @return const T* Protobuf消息对象的指针，如果类型不匹配则返回nullptr
 */
template <typename T> const T* getProtobufMessageSafe(MessagePtr message, bool* ok = nullptr)
{
    static const T defaultValue;
    auto typedMessage = qSharedPointerDynamicCast<ProtobufMessage<T>>(message);
    if (ok)
        *ok = (typedMessage != nullptr);
    return typedMessage ? &(typedMessage->getMessage()) : nullptr;
}

}  // namespace QtWebSocketProtobuf
#endif  // QTWEBSOCKETPROTOBUF_PROTOBUFMESSAGE_H
