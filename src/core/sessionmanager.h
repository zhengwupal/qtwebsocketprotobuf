#ifndef QTWEBSOCKETPROTOBUF_SESSIONMANAGER_H
#define QTWEBSOCKETPROTOBUF_SESSIONMANAGER_H

#include "../qtwebsocketprotobufglobal.h"
#include <QAtomicInt>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QReadWriteLock>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>

namespace QtWebSocketProtobuf
{

class BroadcastResult;

/**
 * @brief WebSocket会话管理器
 *
 * 负责管理WebSocket连接的生命周期，提供线程安全的会话管理
 */
class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);
    ~SessionManager();

    quint64 createSession(QWebSocket* socket);
    bool restoreSession(QWebSocket* socket, quint64 sessionId);
    void removeSession(quint64 sessionId);
    void removeSession(QWebSocket* socket);
    void disconnectSession(quint64 sessionId, const QString& reason = QString());

    QWebSocket* getSocket(quint64 sessionId) const;
    quint64 getSessionId(QWebSocket* socket) const;
    QList<quint64> getAllSessionIds() const;
    int sessionCount() const;
    bool hasSession(quint64 sessionId) const;
    bool hasSession(QWebSocket* socket) const;
    void setSessionTimeout(int timeoutMs);
    int sessionTimeout() const;
    void updateSessionActivity(quint64 sessionId);

    quint32 getNextMessageId(quint64 sessionId);

    qint64 sendBinaryMessage(quint64 sessionId, const QByteArray& data);
    BroadcastResult broadcastBinaryMessage(const QByteArray& data);

signals:
    void sessionCreated(quint64 sessionId);
    void sessionRemoved(quint64 sessionId);
    void sessionRestored(quint64 sessionId);
    void sessionTimeout(quint64 sessionId);

private slots:
    void cleanupExpiredSessions();

private:
    /**
     * @brief 会话信息结构体
     *
     * 存储单个WebSocket会话的详细信息
     */
    struct Session
    {
        quint64 sessionId;
        QWebSocket* socket;
        QAtomicInt nextMessageId = 0;
        QDateTime lastActivity;
        bool isActive = true;

        Session()
            : sessionId(0), socket(nullptr), nextMessageId(0), lastActivity(QDateTime::currentDateTime()),
              isActive(true)
        {
        }
        Session(quint64 id, QWebSocket* sock)
            : sessionId(id), socket(sock), nextMessageId(0), lastActivity(QDateTime::currentDateTime()), isActive(true)
        {
        }
    };

    QMap<quint64, Session*> m_sessions;

    int m_sessionCount;
    mutable QReadWriteLock m_sessionsLock;

    int m_sessionTimeoutMs = 300000;  // 5分钟超时
    QTimer* m_cleanupTimer = nullptr;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_SESSIONMANAGER_H
