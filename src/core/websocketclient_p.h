#ifndef QTWEBSOCKETPROTOBUF_WEBSOCKETCLIENT_P_H
#define QTWEBSOCKETPROTOBUF_WEBSOCKETCLIENT_P_H

#include "abstractmessagerouter.h"
#include "websocketclient.h"
#include <QAtomicInt>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <functional>

namespace QtWebSocketProtobuf
{

class WebSocketClientPrivate
{
    Q_DECLARE_PUBLIC(WebSocketClient)

public:
    WebSocketClientPrivate(WebSocketClient* q, AbstractMessageRouter* router) : q_ptr(q)
    {
        webSocket = new QWebSocket(QStringLiteral("WebSocketClient"), QWebSocketProtocol::VersionLatest, q);
        messageRouter = router;
        reconnectTimer.setSingleShot(true);
    }

    WebSocketClient* const q_ptr;
    QWebSocket* webSocket;
    AbstractMessageRouter* messageRouter;
    QTimer reconnectTimer;
    QUrl serverUrl;
    QAbstractSocket::SocketState state = QAbstractSocket::UnconnectedState;
    quint64 sessionId = 0;
    quint64 savedSessionId = 0;
    QAtomicInt m_nextMessageId = 0;

    // 重连相关配置
    bool autoReconnect = false;
    int reconnectInterval = 5000;
    int maxReconnectAttempts = 5;
    int currentReconnectAttempts = 0;

    // 连接超时相关配置
    QTimer connectionTimeoutTimer;
    int connectionTimeoutMs = 5000;
    bool connectionTimeoutEnabled = true;

    WebSocketClient::StateChangedCallback stateCallback;
    WebSocketClient::ErrorCallback errorCallback;
    std::function<void()> connectionTimeoutCallback;

    mutable QMutex m_mutex;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_WEBSOCKETCLIENT_P_H
