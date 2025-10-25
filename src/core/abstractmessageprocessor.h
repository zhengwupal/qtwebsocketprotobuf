#ifndef QTWEBSOCKETPROTOBUF_ABSTRACTMESSAGEPROCESSOR_H
#define QTWEBSOCKETPROTOBUF_ABSTRACTMESSAGEPROCESSOR_H

#include "../qtwebsocketprotobufglobal.h"
#include "message.h"
#include <QByteArray>
#include <QObject>
#include <QString>

namespace QtWebSocketProtobuf
{

/**
 * @brief 消息处理器接口类
 *
 * 负责处理特定类型的消息内容
 */
class QTWEBSOCKETPROTOBUF_EXPORT AbstractMessageProcessor : public QObject
{
    Q_OBJECT

public:
    explicit AbstractMessageProcessor(QObject* parent = nullptr);
    virtual ~AbstractMessageProcessor();

    /**
     * @brief 处理消息
     *
     * @param data 消息数据
     * @return 处理是否成功
     */
    virtual bool processMessage(const QByteArray& data) = 0;

    /**
     * @brief 序列化消息
     *
     * @param message 消息对象
     * @return 序列化后的数据
     */
    virtual QByteArray serializeMessage(MessagePtr message) = 0;
};

using AbstractMessageProcessorPtr = QSharedPointer<AbstractMessageProcessor>;

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_ABSTRACTMESSAGEPROCESSOR_H
