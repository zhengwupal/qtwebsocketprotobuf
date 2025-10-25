#ifndef QTWEBSOCKETPROTOBUF_MESSAGESERIALIZER_H
#define QTWEBSOCKETPROTOBUF_MESSAGESERIALIZER_H

#include "../qtwebsocketprotobufglobal.h"
#include <google/protobuf/message.h>
#include <QByteArray>

namespace QtWebSocketProtobuf
{

/**
 * @brief Protobuf消息序列化和反序列化工具类
 *
 * 提供统一的Protobuf消息序列化和反序列化接口
 */
class QTWEBSOCKETPROTOBUF_EXPORT MessageSerializer
{
public:
    static QByteArray serializeMessage(const google::protobuf::Message& message);
    static bool deserializeMessage(const QByteArray& data, google::protobuf::Message& message);
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_MESSAGESERIALIZER_H
