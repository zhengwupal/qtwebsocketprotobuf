#ifndef EXAMPLEMESSAGEPROCESSOR_H
#define EXAMPLEMESSAGEPROCESSOR_H

#include "example_message.pb.h"
#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QtWebSocketProtobuf/AbstractMessageProcessor>

/**
 * @brief 消息处理器
 *
 * 处理Protobuf消息
 */
class ExampleMessageProcessor : public QtWebSocketProtobuf::AbstractMessageProcessor
{
    Q_OBJECT

public:
    explicit ExampleMessageProcessor(QObject* parent = nullptr);
    virtual ~ExampleMessageProcessor() override;

    bool processMessage(const QByteArray& data) override;

    QByteArray createEchoMessage(const QString& content, quint64 sessionId);
    QByteArray createStatusMessage(example::StatusMessage::StatusCode code, const QString& message,
                                   const QString& details, quint64 sessionId);
    QByteArray createGoodbyeMessage(const QString& reason, quint64 sessionId);
    QByteArray createWelcomeMessage(quint64 sessionId);

    using EchoCallback = std::function<void(const example::EchoMessage&, quint64 sessionId, quint32 messageId)>;
    using StatusCallback = std::function<void(const example::StatusMessage&)>;
    using GoodbyeCallback = std::function<void(const example::GoodbyeMessage&)>;
    using LatencyTestCallback =
        std::function<void(const example::LatencyTestMessage&, quint64 sessionId, quint32 messageId)>;

    void setEchoCallback(EchoCallback callback);
    void setStatusCallback(StatusCallback callback);
    void setGoodbyeCallback(GoodbyeCallback callback);
    void setLatencyTestCallback(LatencyTestCallback callback);

signals:
    void echoReceived(const example::EchoMessage& message);
    void statusReceived(const example::StatusMessage& message);
    void goodbyeReceived(const example::GoodbyeMessage& message);
    void latencyTestReceived(const example::LatencyTestMessage& message);
    void connAckReceived(quint64 sessionId);
    void ackReceived(const example::AckMessage& message);

private:
    bool processEchoMessage(const QByteArray& payload, quint64 sessionId, quint32 messageId);
    bool processStatusMessage(const QByteArray& payload);
    bool processGoodbyeMessage(const QByteArray& payload);
    bool processLatencyTestMessage(const QByteArray& payload, quint64 sessionId, quint32 messageId);
    bool processConnAckMessage(const QByteArray& payload, quint64 sessionId);
    bool processAckMessage(const QByteArray& payload);

    EchoCallback echoCallback;
    StatusCallback statusCallback;
    GoodbyeCallback goodbyeCallback;
    LatencyTestCallback latencyTestCallback;
};

#endif  // EXAMPLEMESSAGEPROCESSOR_H
