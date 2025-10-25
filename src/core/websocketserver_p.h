#ifndef QTWEBSOCKETPROTOBUF_WEBSOCKETSERVER_P_H
#define QTWEBSOCKETPROTOBUF_WEBSOCKETSERVER_P_H

#include "abstractmessagerouter.h"
#include "sessionmanager.h"
#include "websocketserver.h"
#include <QHostAddress>
#include <QObject>
#include <QStringList>
#include <QWebSocket>
#include <QWebSocketServer>
#include <functional>

namespace QtWebSocketProtobuf
{

class WebSocketServerPrivate
{
    Q_DECLARE_PUBLIC(WebSocketServer)

public:
    WebSocketServerPrivate(WebSocketServer* q, AbstractMessageRouter* router) : q_ptr(q)
    {
        server = new QWebSocketServer(QStringLiteral("WebSocketServer"), QWebSocketServer::NonSecureMode, q);
        messageRouter = router;
        sessionManager = new SessionManager(q);
    }

    WebSocketServer* const q_ptr;
    QWebSocketServer* server;
    AbstractMessageRouter* messageRouter;
    SessionManager* sessionManager;

    WebSocketServer::SessionConnectedCallback sessionConnectedCallback;
    WebSocketServer::SessionDisconnectedCallback sessionDisconnectedCallback;
    WebSocketServer::ServerErrorCallback serverErrorCallback;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_WEBSOCKETSERVER_P_H
