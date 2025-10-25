#include "examplemessageprocessor.h"
#include "example_message.pb.h"
#include "messagetypeutils.h"
#include "messagevalidationutils.h"
#include <QDateTime>
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/MessageSerializer>
#include <QtWebSocketProtobuf/PacketSerializer>
#include <QtWebSocketProtobuf/ProtobufMessage>

ExampleMessageProcessor::ExampleMessageProcessor(QObject* parent) : AbstractMessageProcessor(parent) {}

ExampleMessageProcessor::~ExampleMessageProcessor() {}

bool ExampleMessageProcessor::processMessage(const QByteArray& data)
{
    if (!MessageValidationUtils::isValidMessageSize(data.size())) {
        LOG_ERROR(QString("Input message too large: %1 bytes (max: %2)")
                      .arg(data.size())
                      .arg(MessageValidationUtils::getMaxMessageSizeDescription()));
        return false;
    }

    quint32 payloadSize, messageId;
    quint16 messageType;
    quint64 sessionId;

    if (!QtWebSocketProtobuf::PacketSerializer::extractHeader(data, payloadSize, messageType, messageId, sessionId)) {
        LOG_ERROR("Failed to extract packet header");
        return false;
    }

    if (!MessageValidationUtils::isValidPayloadSize(payloadSize)) {
        LOG_ERROR(QString("Payload too large: %1 bytes (max: %2 bytes)")
                      .arg(payloadSize)
                      .arg(MessageValidationUtils::MAX_PAYLOAD_SIZE));
        return false;
    }

    if (!MessageTypeUtils::isValid(messageType)) {
        LOG_ERROR(QString("Invalid message type: %1 (valid range: %2)")
                      .arg(messageType)
                      .arg(MessageTypeUtils::getValidRangeDescription()));
        return false;
    }

    QByteArray payload;
    if (!QtWebSocketProtobuf::PacketSerializer::extractPayload(data, payloadSize, payload)) {
        LOG_ERROR("Failed to extract payload");
        return false;
    }

    switch (messageType) {
    case example::MessageType::CONNACK:
        return processConnAckMessage(payload, sessionId);
    case example::MessageType::STATUS:
        return processStatusMessage(payload);
    case example::MessageType::ACK:
        return processAckMessage(payload);
    case example::MessageType::ECHO:
        return processEchoMessage(payload, sessionId, messageId);
    case example::MessageType::GOODBYE:
        return processGoodbyeMessage(payload);
    case example::MessageType::LATENCY_TEST:
        return processLatencyTestMessage(payload, sessionId, messageId);
    default:
        LOG_WARNING(QString("Unknown message type: %1").arg(messageType));
        return false;
    }
}

QByteArray ExampleMessageProcessor::createEchoMessage(const QString& content, quint64 sessionId)
{
    example::EchoMessage message;
    message.set_content(content.toStdString());

    auto messagePtr = QtWebSocketProtobuf::createMessage(message, example::MessageType::ECHO, sessionId);
    return messagePtr->serialize();
}

QByteArray ExampleMessageProcessor::createStatusMessage(example::StatusMessage::StatusCode code, const QString& message,
                                                        const QString& details, quint64 sessionId)
{
    example::StatusMessage statusMessage;
    statusMessage.set_code(code);
    statusMessage.set_message(message.toStdString());

    if (!details.isEmpty()) {
        statusMessage.set_details(details.toStdString());
    }

    auto messagePtr = QtWebSocketProtobuf::createMessage(statusMessage, example::MessageType::STATUS, sessionId);
    return messagePtr->serialize();
}

QByteArray ExampleMessageProcessor::createGoodbyeMessage(const QString& reason, quint64 sessionId)
{
    example::GoodbyeMessage message;
    message.set_reason(reason.toStdString());

    auto messagePtr = QtWebSocketProtobuf::createMessage(message, example::MessageType::GOODBYE, sessionId);
    return messagePtr->serialize();
}

QByteArray ExampleMessageProcessor::createWelcomeMessage(quint64 sessionId)
{
    example::StatusMessage welcomeMsg;
    welcomeMsg.set_code(example::StatusMessage::OK);
    welcomeMsg.set_message("Welcome to WebSocket Protobuf Server!");
    welcomeMsg.set_details(QString("Your session ID: %1").arg(sessionId).toStdString());

    auto messagePtr = QtWebSocketProtobuf::createMessage(welcomeMsg, example::MessageType::STATUS, sessionId);
    return messagePtr->serialize();
}

void ExampleMessageProcessor::setEchoCallback(EchoCallback callback)
{
    echoCallback = callback;
}

void ExampleMessageProcessor::setStatusCallback(StatusCallback callback)
{
    statusCallback = callback;
}

void ExampleMessageProcessor::setGoodbyeCallback(GoodbyeCallback callback)
{
    goodbyeCallback = callback;
}

void ExampleMessageProcessor::setLatencyTestCallback(LatencyTestCallback callback)
{
    latencyTestCallback = callback;
}

bool ExampleMessageProcessor::processEchoMessage(const QByteArray& payload, quint64 sessionId, quint32 messageId)
{
    example::EchoMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(payload, message)) {
        LOG_ERROR("Failed to deserialize EchoMessage");
        return false;
    }

    emit echoReceived(message);

    if (echoCallback) {
        echoCallback(message, sessionId, messageId);
    }
    return true;
}

bool ExampleMessageProcessor::processStatusMessage(const QByteArray& payload)
{
    example::StatusMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(payload, message)) {
        LOG_ERROR("Failed to deserialize StatusMessage");
        return false;
    }

    emit statusReceived(message);

    if (statusCallback) {
        statusCallback(message);
    }
    return true;
}

bool ExampleMessageProcessor::processGoodbyeMessage(const QByteArray& payload)
{
    example::GoodbyeMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(payload, message)) {
        LOG_ERROR("Failed to deserialize GoodbyeMessage");
        return false;
    }

    emit goodbyeReceived(message);

    if (goodbyeCallback) {
        goodbyeCallback(message);
    }
    return true;
}

bool ExampleMessageProcessor::processLatencyTestMessage(const QByteArray& payload, quint64 sessionId, quint32 messageId)
{
    example::LatencyTestMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(payload, message)) {
        LOG_ERROR("Failed to deserialize LatencyTestMessage");
        return false;
    }

    emit latencyTestReceived(message);

    if (latencyTestCallback) {
        latencyTestCallback(message, sessionId, messageId);
    }
    return true;
}

bool ExampleMessageProcessor::processConnAckMessage(const QByteArray& payload, quint64 sessionId)
{
    example::ConnAckMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(payload, message)) {
        LOG_ERROR("Failed to deserialize ConnAckMessage");
        return false;
    }

    if (sessionId != 0) {
        emit connAckReceived(sessionId);
    } else {
        LOG_WARNING("Received ConnAckMessage with empty sessionId!");
    }
    return true;
}

bool ExampleMessageProcessor::processAckMessage(const QByteArray& payload)
{
    example::AckMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(payload, message)) {
        LOG_ERROR("Failed to deserialize AckMessage");
        return false;
    }

    emit ackReceived(message);
    return true;
}
