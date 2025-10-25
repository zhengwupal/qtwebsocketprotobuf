#ifndef QTWEBSOCKETPROTOBUF_SNOWFLAKEID_H
#define QTWEBSOCKETPROTOBUF_SNOWFLAKEID_H

#include "../qtwebsocketprotobufglobal.h"
#include <QAtomicInt>
#include <QDateTime>
#include <QMutex>

namespace QtWebSocketProtobuf
{

/**
 * @brief 雪花算法ID生成器
 *
 * 生成64位唯一ID，包含时间戳、机器ID (0-1023)、序列号 (0-4095)
 * 格式：1位符号位 + 41位时间戳 + 10位机器ID + 12位序列号
 */
class QTWEBSOCKETPROTOBUF_EXPORT SnowflakeIdGenerator
{
public:
    explicit SnowflakeIdGenerator(quint16 machineId = 0);

    quint64 nextId();
    quint16 getMachineId() const
    {
        return m_machineId;
    }

    void setMachineId(quint16 machineId);
    static qint64 extractTimestamp(quint64 id);
    static quint16 extractMachineId(quint64 id);
    static quint16 extractSequence(quint64 id);

private:
    static constexpr qint64 EPOCH = 1609459200000LL;  // 2021-01-01 00:00:00 UTC
    static constexpr qint64 MACHINE_ID_BITS = 10;
    static constexpr qint64 SEQUENCE_BITS = 12;

    static constexpr qint64 MAX_MACHINE_ID = (1LL << MACHINE_ID_BITS) - 1;
    static constexpr qint64 MAX_SEQUENCE = (1LL << SEQUENCE_BITS) - 1;

    static constexpr qint64 MACHINE_ID_SHIFT = SEQUENCE_BITS;
    static constexpr qint64 TIMESTAMP_SHIFT = SEQUENCE_BITS + MACHINE_ID_BITS;

    qint64 getCurrentTimestamp() const;
    qint64 waitNextMillis(qint64 lastTimestamp) const;

    quint16 m_machineId;
    qint64 m_lastTimestamp;
    quint16 m_sequence;
    QMutex m_mutex;
};

/**
 * @brief 全局雪花ID生成器实例
 */
QTWEBSOCKETPROTOBUF_EXPORT SnowflakeIdGenerator& getSnowflakeIdGenerator();

/**
 * @brief 生成下一个会话ID
 * @return 64位会话ID
 */
QTWEBSOCKETPROTOBUF_EXPORT quint64 generateSessionId();

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_SNOWFLAKEID_H
