#ifndef QTWEBSOCKETPROTOBUF_ABSTRACTMESSAGEROUTER_H
#define QTWEBSOCKETPROTOBUF_ABSTRACTMESSAGEROUTER_H

#include "../qtwebsocketprotobufglobal.h"
#include "abstractmessageprocessor.h"
#include <QList>
#include <QObject>

namespace QtWebSocketProtobuf
{

/**
 * @brief 消息路由器接口类
 *
 * 负责管理和分发消息到对应的处理器
 */
class QTWEBSOCKETPROTOBUF_EXPORT AbstractMessageRouter : public QObject
{
    Q_OBJECT

public:
    explicit AbstractMessageRouter(QObject* parent = nullptr);
    virtual ~AbstractMessageRouter();

    /**
     * @brief 注册消息处理器
     *
     * @param processor 消息处理器实例
     * @param messageTypeIds 该处理器支持的消息类型ID列表
     */
    virtual void registerProcessor(AbstractMessageProcessorPtr processor, const QList<int>& messageTypeIds) = 0;

    /**
     * @brief 注销消息处理器
     *
     * @param messageTypeIds 要注销的消息类型ID列表
     */
    virtual void unregisterProcessor(const QList<int>& messageTypeIds) = 0;

    /**
     * @brief 处理接收到的消息（路由到对应的处理器）
     *
     * @param data 消息数据
     */
    virtual void processMessage(const QByteArray& data) const = 0;

    /**
     * @brief 序列化要发送的消息（通过对应的处理器）
     *
     * @param message 要发送的消息
     * @return 序列化后的消息数据
     */
    virtual QByteArray serializeMessage(MessagePtr message) const = 0;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_ABSTRACTMESSAGEROUTER_H
