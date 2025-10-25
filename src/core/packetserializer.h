#ifndef QTWEBSOCKETPROTOBUF_PACKETSERIALIZER_H
#define QTWEBSOCKETPROTOBUF_PACKETSERIALIZER_H

#include "../qtwebsocketprotobufglobal.h"
#include "packetheader.h"
#include <QByteArray>

namespace QtWebSocketProtobuf
{

/**
 * @brief 数据包序列化类
 *
 * 提供数据包的序列化和提取功能
 */
class QTWEBSOCKETPROTOBUF_EXPORT PacketSerializer
{
public:
    static QByteArray serializePacket(quint32 payloadSize, quint16 messageType, quint32 messageId, quint64 sessionId,
                                      const QByteArray& payload);
    static bool extractHeader(const QByteArray& packet, quint32& payloadSize, quint16& messageType, quint32& messageId,
                              quint64& sessionId);
    static bool extractPayload(const QByteArray& packet, quint32 payloadSize, QByteArray& payload);
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_PACKETSERIALIZER_H
