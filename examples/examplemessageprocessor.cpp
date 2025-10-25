#include "examplemessageprocessor.h"
#include "example_message.pb.h"
#include "messagetypeutils.h"
#include "messagevalidationutils.h"
#include <QDateTime>
#include <QUuid>
#include <QtWebSocketProtobuf/Logger>
#include <QtWebSocketProtobuf/Message>
#include <QtWebSocketProtobuf/MessageSerializer>
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

    example::MessageWrapper wrapper;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(data, wrapper)) {
        LOG_ERROR("Failed to deserialize MessageWrapper");
        return false;
    }

    QString sessionId = QString::fromStdString(wrapper.session_id());
    if (!MessageValidationUtils::isValidSessionIdLength(sessionId.length())) {
        LOG_ERROR(QString("Session ID length invalid: %1 characters (max: %2)")
                      .arg(sessionId.length())
                      .arg(MessageValidationUtils::MAX_SESSION_ID_LENGTH));
        return false;
    }

    QByteArray payload = QByteArray::fromStdString(wrapper.payload());
    if (!MessageValidationUtils::isValidPayloadSize(payload.size())) {
        LOG_ERROR(QString("Payload too large: %1 bytes (max: %2 bytes)")
                      .arg(payload.size())
                      .arg(MessageValidationUtils::MAX_PAYLOAD_SIZE));
        return false;
    }

    int messageType = static_cast<int>(wrapper.type());
    if (!MessageTypeUtils::isValid(messageType)) {
        LOG_ERROR(QString("Invalid message type: %1 (valid range: %2)")
                      .arg(messageType)
                      .arg(MessageTypeUtils::getValidRangeDescription()));
        return false;
    }

    quint32 messageId = wrapper.message_id();
    switch (wrapper.type()) {
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
        LOG_WARNING(QString("Unknown message type ID: %1").arg(wrapper.type()));
        return false;
    }
}

QByteArray ExampleMessageProcessor::serializeMessage(QtWebSocketProtobuf::MessagePtr message)
{
    if (!message) {
        LOG_ERROR("Cannot serialize null message");
        return QByteArray();
    }

    int typeId = message->typeId();
    if (!MessageTypeUtils::isValid(typeId)) {
        LOG_ERROR(QString("Invalid message type: %1 (valid range: %2)")
                      .arg(typeId)
                      .arg(MessageTypeUtils::getValidRangeDescription()));
        return QByteArray();
    }

    QString sessionId = message->sessionId();
    if (!MessageValidationUtils::isValidSessionIdLength(sessionId.length())) {
        LOG_ERROR(QString("Session ID length invalid: %1 characters (max: %2)")
                      .arg(sessionId.length())
                      .arg(MessageValidationUtils::MAX_SESSION_ID_LENGTH));
        return QByteArray();
    }

    example::MessageWrapper wrapper;
    wrapper.set_type(static_cast<example::MessageType>(typeId));
    wrapper.set_session_id(sessionId.toStdString());

    quint32 messageId = message->messageId();
    if (messageId == 0) {
        messageId = 1;
    }
    wrapper.set_message_id(messageId);

    QByteArray payload;
    switch (typeId) {
    case example::MessageType::CONNACK: {
        const auto* connAckMsg = QtWebSocketProtobuf::getProtobufMessageSafe<example::ConnAckMessage>(message);
        if (!connAckMsg) {
            LOG_ERROR("Failed to convert message type: expected ConnAckMessage");
            return QByteArray();
        }
        payload = QtWebSocketProtobuf::MessageSerializer::serializeMessage(*connAckMsg);
        break;
    }
    case example::MessageType::STATUS: {
        const auto* statusMsg = QtWebSocketProtobuf::getProtobufMessageSafe<example::StatusMessage>(message);
        if (!statusMsg) {
            LOG_ERROR("Failed to convert message type: expected StatusMessage");
            return QByteArray();
        }
        payload = QtWebSocketProtobuf::MessageSerializer::serializeMessage(*statusMsg);
        break;
    }
    case example::MessageType::ACK: {
        const auto* ackMsg = QtWebSocketProtobuf::getProtobufMessageSafe<example::AckMessage>(message);
        if (!ackMsg) {
            LOG_ERROR("Failed to convert message type: expected AckMessage");
            return QByteArray();
        }
        payload = QtWebSocketProtobuf::MessageSerializer::serializeMessage(*ackMsg);
        break;
    }
    case example::MessageType::ECHO: {
        const auto* echoMsg = QtWebSocketProtobuf::getProtobufMessageSafe<example::EchoMessage>(message);
        if (!echoMsg) {
            LOG_ERROR("Failed to convert message type: expected EchoMessage");
            return QByteArray();
        }
        payload = QtWebSocketProtobuf::MessageSerializer::serializeMessage(*echoMsg);
        break;
    }
    case example::MessageType::GOODBYE: {
        const auto* goodbyeMsg = QtWebSocketProtobuf::getProtobufMessageSafe<example::GoodbyeMessage>(message);
        if (!goodbyeMsg) {
            LOG_ERROR("Failed to convert message type: expected GoodbyeMessage");
            return QByteArray();
        }
        payload = QtWebSocketProtobuf::MessageSerializer::serializeMessage(*goodbyeMsg);
        break;
    }
    case example::MessageType::LATENCY_TEST: {
        const auto* latencyMsg = QtWebSocketProtobuf::getProtobufMessageSafe<example::LatencyTestMessage>(message);
        if (!latencyMsg) {
            LOG_ERROR("Failed to convert message type: expected LatencyTestMessage");
            return QByteArray();
        }
        payload = QtWebSocketProtobuf::MessageSerializer::serializeMessage(*latencyMsg);
        break;
    }
    default:
        LOG_ERROR(QString("Unknown message type: %1").arg(typeId));
        return QByteArray();
    }

    if (!MessageValidationUtils::isValidPayloadSize(payload.size())) {
        LOG_ERROR(QString("Payload too large: %1 bytes (max: %2 bytes)")
                      .arg(payload.size())
                      .arg(MessageValidationUtils::MAX_PAYLOAD_SIZE));
        return QByteArray();
    }

    wrapper.set_payload(payload.toStdString());
    QByteArray finalMessage = QtWebSocketProtobuf::MessageSerializer::serializeMessage(wrapper);
    if (!MessageValidationUtils::isValidMessageSize(finalMessage.size())) {
        LOG_ERROR(QString("Final message too large: %1 bytes (max: %2)")
                      .arg(finalMessage.size())
                      .arg(MessageValidationUtils::getMaxMessageSizeDescription()));
        return QByteArray();
    }

    return finalMessage;
}

QByteArray ExampleMessageProcessor::createEchoMessage(const QString& content, const QString& sessionId)
{
    example::EchoMessage message;
    message.set_content(content.toStdString());

    auto messagePtr = QtWebSocketProtobuf::createMessage(message, example::MessageType::ECHO, sessionId);
    return serializeMessage(messagePtr);
}

QByteArray ExampleMessageProcessor::createStatusMessage(example::StatusMessage::StatusCode code, const QString& message,
                                                        const QString& details, const QString& sessionId)
{
    example::StatusMessage statusMessage;
    statusMessage.set_code(code);
    statusMessage.set_message(message.toStdString());

    if (!details.isEmpty()) {
        statusMessage.set_details(details.toStdString());
    }

    auto messagePtr = QtWebSocketProtobuf::createMessage(statusMessage, example::MessageType::STATUS, sessionId);
    return serializeMessage(messagePtr);
}

QByteArray ExampleMessageProcessor::createGoodbyeMessage(const QString& reason, const QString& sessionId)
{
    example::GoodbyeMessage message;
    message.set_reason(reason.toStdString());

    auto messagePtr = QtWebSocketProtobuf::createMessage(message, example::MessageType::GOODBYE, sessionId);
    return serializeMessage(messagePtr);
}

QByteArray ExampleMessageProcessor::createWelcomeMessage(const QString& sessionId)
{
    example::StatusMessage welcomeMsg;
    welcomeMsg.set_code(example::StatusMessage::OK);
    welcomeMsg.set_message("Welcome to WebSocket Protobuf Server!");
    welcomeMsg.set_details(("Your session ID: " + sessionId).toStdString());

    auto messagePtr = QtWebSocketProtobuf::createMessage(welcomeMsg, example::MessageType::STATUS, sessionId);
    return serializeMessage(messagePtr);
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

bool ExampleMessageProcessor::processEchoMessage(const QByteArray& data, const QString& sessionId, quint32 messageId)
{
    example::EchoMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(data, message)) {
        LOG_ERROR("Failed to deserialize EchoMessage");
        return false;
    }
    emit echoReceived(message);

    if (echoCallback) {
        echoCallback(message, sessionId, messageId);
    }
    return true;
}

bool ExampleMessageProcessor::processStatusMessage(const QByteArray& data)
{
    example::StatusMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(data, message)) {
        LOG_ERROR("Failed to deserialize StatusMessage");
        return false;
    }
    emit statusReceived(message);

    if (statusCallback) {
        statusCallback(message);
    }
    return true;
}

bool ExampleMessageProcessor::processGoodbyeMessage(const QByteArray& data)
{
    example::GoodbyeMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(data, message)) {
        LOG_ERROR("Failed to deserialize GoodbyeMessage");
        return false;
    }
    emit goodbyeReceived(message);

    if (goodbyeCallback) {
        goodbyeCallback(message);
    }
    return true;
}

bool ExampleMessageProcessor::processLatencyTestMessage(const QByteArray& data, const QString& sessionId,
                                                        quint32 messageId)
{
    example::LatencyTestMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(data, message)) {
        LOG_ERROR("Failed to deserialize LatencyTestMessage");
        return false;
    }
    emit latencyTestReceived(message);

    if (latencyTestCallback) {
        latencyTestCallback(message, sessionId, messageId);
    }
    return true;
}

bool ExampleMessageProcessor::processConnAckMessage(const QByteArray& data, const QString& sessionId)
{
    example::ConnAckMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(data, message)) {
        LOG_ERROR("Failed to deserialize ConnAckMessage");
        return false;
    }

    if (!sessionId.isEmpty()) {
        emit connAckReceived(sessionId);
    } else {
        LOG_WARNING("Received ConnAckMessage with empty sessionId!");
    }
    return true;
}

bool ExampleMessageProcessor::processAckMessage(const QByteArray& data)
{
    example::AckMessage message;
    if (!QtWebSocketProtobuf::MessageSerializer::deserializeMessage(data, message)) {
        LOG_ERROR("Failed to deserialize AckMessage");
        return false;
    }

    emit ackReceived(message);
    return true;
}
