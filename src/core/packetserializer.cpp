#include "packetserializer.h"
#include "packetheader.h"
#include <QDataStream>
#include <QDebug>

namespace QtWebSocketProtobuf
{

QByteArray PacketSerializer::serializePacket(quint32 payloadSize, quint16 messageType, quint32 messageId,
                                             quint64 sessionId, const QByteArray& payload)
{
    QByteArray packet;
    packet.append(PacketHeader::toByteArray(payloadSize, messageType, messageId, sessionId));
    packet.append(payload);

    // 计算整个数据包的CRC（header + payload）
    quint32 crc = PacketHeader::calculateCrc32(packet);

    // 将CRC追加到数据包末尾
    QByteArray crcBytes;
    QDataStream crcStream(&crcBytes, QIODevice::WriteOnly);
    crcStream.setByteOrder(QDataStream::LittleEndian);
    crcStream << crc;

    packet.append(crcBytes);

    return packet;
}

bool PacketSerializer::extractHeader(const QByteArray& packet, quint32& payloadSize, quint16& messageType,
                                     quint32& messageId, quint64& sessionId)
{
    if (packet.size() < static_cast<int>(PacketHeader::HEADER_SIZE)) {
        return false;
    }

    return PacketHeader::fromByteArray(packet.left(PacketHeader::HEADER_SIZE), payloadSize, messageType, messageId,
                                       sessionId);
}

bool PacketSerializer::extractPayload(const QByteArray& packet, quint32 payloadSize, QByteArray& payload)
{
    if (packet.size() < static_cast<int>(PacketHeader::HEADER_SIZE)) {
        return false;
    }

    if (packet.size() < static_cast<int>(PacketHeader::HEADER_SIZE + payloadSize)) {
        return false;
    }

    payload = packet.mid(PacketHeader::HEADER_SIZE, payloadSize);
    return true;
}

}  // namespace QtWebSocketProtobuf
