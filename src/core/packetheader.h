#ifndef QTWEBSOCKETPROTOBUF_PACKETHEADER_H
#define QTWEBSOCKETPROTOBUF_PACKETHEADER_H

#include "../qtwebsocketprotobufglobal.h"
#include <QByteArray>
#include <QDataStream>

namespace QtWebSocketProtobuf
{

/**
 * @brief 数据包头部类
 *
 * 提供数据包头部的序列化和反序列化功能
 */
class QTWEBSOCKETPROTOBUF_EXPORT PacketHeader
{
public:
    static constexpr size_t HEADER_SIZE = 24;

    static QByteArray toByteArray(quint32 payloadSize, quint16 messageType, quint32 messageId, quint64 sessionId);
    static bool fromByteArray(const QByteArray& data, quint32& payloadSize, quint16& messageType, quint32& messageId,
                              quint64& sessionId);
    static quint32 calculateCrc32(const QByteArray& data);
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_PACKETHEADER_H
