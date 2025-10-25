#include "abstractmessageprocessor.h"
#include "message.h"

namespace QtWebSocketProtobuf
{

AbstractMessageProcessor::AbstractMessageProcessor(QObject* parent) : QObject(parent) {}

AbstractMessageProcessor::~AbstractMessageProcessor() {}

}  // namespace QtWebSocketProtobuf
