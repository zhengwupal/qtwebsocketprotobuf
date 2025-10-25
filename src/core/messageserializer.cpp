#include "messageserializer.h"
#include "../utils/logger.h"
#include <google/protobuf/message.h>
#include <QDataStream>

namespace QtWebSocketProtobuf
{

QByteArray MessageSerializer::serializeMessage(const google::protobuf::Message& message)
{
    std::string messageData;
    if (!message.SerializeToString(&messageData)) {
        LOG_ERROR("Failed to serialize protobuf message");
        return QByteArray();
    }

    return QByteArray(messageData.data(), static_cast<int>(messageData.size()));
}

bool MessageSerializer::deserializeMessage(const QByteArray& data, google::protobuf::Message& message)
{
    if (!message.ParseFromArray(data.data(), data.size())) {
        LOG_ERROR("Failed to parse protobuf message");
        return false;
    }

    return true;
}

}  // namespace QtWebSocketProtobuf
