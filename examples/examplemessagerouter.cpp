#include "examplemessagerouter.h"
#include "messagetypeutils.h"
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/Message>
#include <QtWebSocketProtobuf/MessageSerializer>

ExampleMessageRouter::ExampleMessageRouter(QObject* parent) : QtWebSocketProtobuf::AbstractMessageRouter(parent) {}

ExampleMessageRouter::~ExampleMessageRouter() {}

void ExampleMessageRouter::registerProcessor(QtWebSocketProtobuf::AbstractMessageProcessorPtr processor,
                                             const QList<int>& messageTypeIds)
{
    if (!processor) {
        LOG_ERROR("Cannot register null processor");
        return;
    }

    for (int typeId : messageTypeIds) {
        if (m_processors.contains(typeId)) {
            QString typeName = MessageTypeUtils::toString(typeId);
            LOG_WARNING(QString("Processor for %1 already registered, overwriting").arg(typeName));
        }
        m_processors[typeId] = processor;
        QString typeName = MessageTypeUtils::toString(typeId);
        LOG_DEBUG(QString("Registered processor for %1").arg(typeName));
    }
}

void ExampleMessageRouter::unregisterProcessor(const QList<int>& messageTypeIds)
{
    for (int typeId : messageTypeIds) {
        if (m_processors.remove(typeId) > 0) {
            QString typeName = MessageTypeUtils::toString(typeId);
            LOG_DEBUG(QString("Unregistered processor for %1").arg(typeName));
        }
    }
}

void ExampleMessageRouter::processMessage(const QByteArray& data) const
{
    if (data.isEmpty()) {
        LOG_WARNING("Received empty message data");
        return;
    }

    for (auto it = m_processors.begin(); it != m_processors.end(); ++it) {
        if (it.value()->processMessage(data)) {
            return;
        }
    }

    LOG_ERROR("No processor could handle the message");
}

QByteArray ExampleMessageRouter::serializeMessage(QtWebSocketProtobuf::MessagePtr message) const
{
    if (!message) {
        LOG_ERROR("Cannot route null message");
        return QByteArray();
    }

    return message->serialize();
}
