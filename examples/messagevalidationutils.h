#ifndef MESSAGEVALIDATIONUTILS_H
#define MESSAGEVALIDATIONUTILS_H

#include <QString>

/**
 * @brief 消息验证工具类
 *
 * 提供消息大小、负载大小和会话ID长度的验证功能
 */
class MessageValidationUtils
{
public:
    static constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024;
    static constexpr size_t MAX_PAYLOAD_SIZE = 1024 * 1024;
    static constexpr size_t MAX_SESSION_ID_LENGTH = 64;

    static bool isValidMessageSize(size_t size)
    {
        return size > 0 && size <= MAX_MESSAGE_SIZE;
    }

    static bool isValidPayloadSize(size_t size)
    {
        return size <= MAX_PAYLOAD_SIZE;
    }

    static bool isValidSessionIdLength(size_t length)
    {
        return length > 0 && length <= MAX_SESSION_ID_LENGTH;
    }

    static QString getMaxMessageSizeDescription()
    {
        return QString("%1 bytes (%2 MB)").arg(MAX_MESSAGE_SIZE).arg(MAX_MESSAGE_SIZE / (1024 * 1024));
    }

    static QString getMaxPayloadSizeDescription()
    {
        return QString("%1 bytes (%2 MB)").arg(MAX_PAYLOAD_SIZE).arg(MAX_PAYLOAD_SIZE / (1024 * 1024));
    }

    static QString getMaxSessionIdLengthDescription()
    {
        return QString("%1 characters").arg(MAX_SESSION_ID_LENGTH);
    }
};

#endif  // MESSAGEVALIDATIONUTILS_H
