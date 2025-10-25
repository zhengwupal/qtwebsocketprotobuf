#ifndef EXAMPLEMESSAGEROUTER_H
#define EXAMPLEMESSAGEROUTER_H

#include "examplemessageprocessor.h"
#include <QList>
#include <QMap>
#include <QObject>
#include <QtWebSocketProtobuf/AbstractMessageRouter>

/**
 * @brief 消息路由器
 *
 * 负责将消息路由到对应的处理器
 */
class ExampleMessageRouter : public QtWebSocketProtobuf::AbstractMessageRouter
{
    Q_OBJECT

public:
    explicit ExampleMessageRouter(QObject* parent = nullptr);
    virtual ~ExampleMessageRouter() override;

    void registerProcessor(QtWebSocketProtobuf::AbstractMessageProcessorPtr processor,
                           const QList<int>& messageTypeIds) override;
    void unregisterProcessor(const QList<int>& messageTypeIds) override;
    void processMessage(const QByteArray& data) const override;
    QByteArray serializeMessage(QtWebSocketProtobuf::MessagePtr message) const override;

private:
    QMap<int, QtWebSocketProtobuf::AbstractMessageProcessorPtr> m_processors;
};

#endif  // EXAMPLEMESSAGEROUTER_H
