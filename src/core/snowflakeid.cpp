#include "snowflakeid.h"
#include <QDebug>
#include <QThread>

namespace QtWebSocketProtobuf
{

SnowflakeIdGenerator::SnowflakeIdGenerator(quint16 machineId)
    : m_machineId(machineId & MAX_MACHINE_ID), m_lastTimestamp(0), m_sequence(0)
{
    if (machineId > MAX_MACHINE_ID) {
        qWarning() << "Machine ID" << machineId << "exceeds maximum" << MAX_MACHINE_ID << ", using" << m_machineId;
    }
}

quint64 SnowflakeIdGenerator::nextId()
{
    QMutexLocker locker(&m_mutex);

    qint64 timestamp = getCurrentTimestamp();

    if (timestamp < m_lastTimestamp) {
        qWarning() << "Clock moved backwards. Refusing to generate id for" << (m_lastTimestamp - timestamp)
                   << "milliseconds";
        timestamp = m_lastTimestamp;
    }

    if (timestamp == m_lastTimestamp) {
        m_sequence = (m_sequence + 1) & MAX_SEQUENCE;
        if (m_sequence == 0) {
            timestamp = waitNextMillis(m_lastTimestamp);
        }
    } else {
        m_sequence = 0;
    }

    m_lastTimestamp = timestamp;

    return ((timestamp - EPOCH) << TIMESTAMP_SHIFT) | (m_machineId << MACHINE_ID_SHIFT) | m_sequence;
}

void SnowflakeIdGenerator::setMachineId(quint16 machineId)
{
    QMutexLocker locker(&m_mutex);
    m_machineId = machineId & MAX_MACHINE_ID;
}

qint64 SnowflakeIdGenerator::extractTimestamp(quint64 id)
{
    return (id >> TIMESTAMP_SHIFT) + EPOCH;
}

quint16 SnowflakeIdGenerator::extractMachineId(quint64 id)
{
    return (id >> MACHINE_ID_SHIFT) & MAX_MACHINE_ID;
}

quint16 SnowflakeIdGenerator::extractSequence(quint64 id)
{
    return id & MAX_SEQUENCE;
}

qint64 SnowflakeIdGenerator::getCurrentTimestamp() const
{
    return QDateTime::currentMSecsSinceEpoch();
}

qint64 SnowflakeIdGenerator::waitNextMillis(qint64 lastTimestamp) const
{
    qint64 timestamp = getCurrentTimestamp();
    while (timestamp <= lastTimestamp) {
        QThread::msleep(1);
        timestamp = getCurrentTimestamp();
    }
    return timestamp;
}

static SnowflakeIdGenerator g_snowflakeGenerator;

SnowflakeIdGenerator& getSnowflakeIdGenerator()
{
    return g_snowflakeGenerator;
}

quint64 generateSessionId()
{
    return getSnowflakeIdGenerator().nextId();
}

}  // namespace QtWebSocketProtobuf
