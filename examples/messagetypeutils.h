#ifndef MESSAGETYPEUTILS_H
#define MESSAGETYPEUTILS_H

#include "example_message.pb.h"
#include <QString>

/**
 * @brief 消息类型工具类
 *
 * 提供消息类型ID与字符串之间的转换功能
 */
class MessageTypeUtils
{
public:
    static constexpr int MIN_TYPE = static_cast<int>(example::MessageType::UNKNOWN);
    static constexpr int MAX_TYPE = static_cast<int>(example::MessageType::LATENCY_TEST);

    static QString toString(int typeId)
    {
        switch (static_cast<example::MessageType>(typeId)) {
        case example::MessageType::UNKNOWN:
            return "UNKNOWN";
        case example::MessageType::CONNACK:
            return "CONNACK";
        case example::MessageType::STATUS:
            return "STATUS";
        case example::MessageType::ACK:
            return "ACK";
        case example::MessageType::ECHO:
            return "ECHO";
        case example::MessageType::GOODBYE:
            return "GOODBYE";
        case example::MessageType::LATENCY_TEST:
            return "LATENCY_TEST";
        default:
            return QString("UNKNOWN(%1)").arg(typeId);
        }
    }

    static bool isValid(int typeId)
    {
        return typeId >= MIN_TYPE && typeId <= MAX_TYPE;
    }

    static QString getValidRangeDescription()
    {
        return QString("%1-%2").arg(MIN_TYPE).arg(MAX_TYPE);
    }
};

#endif  // MESSAGETYPEUTILS_H
