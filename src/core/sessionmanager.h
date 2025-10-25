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

    QString createSession(QWebSocket* socket);
    bool restoreSession(QWebSocket* socket, const QString& sessionId);
    void removeSession(const QString& sessionId);
    void removeSession(QWebSocket* socket);
    void disconnectSession(const QString& sessionId, const QString& reason = QString());

    QWebSocket* getSocket(const QString& sessionId) const;
    QString getSessionId(QWebSocket* socket) const;
    QStringList getAllSessionIds() const;
    int sessionCount() const;
    bool hasSession(const QString& sessionId) const;
    bool hasSession(QWebSocket* socket) const;
    void setSessionTimeout(int timeoutMs);
    int sessionTimeout() const;
    void updateSessionActivity(const QString& sessionId);

    quint32 getNextMessageId(const QString& sessionId);

    qint64 sendBinaryMessage(const QString& sessionId, const QByteArray& data);
    BroadcastResult broadcastBinaryMessage(const QByteArray& data);

signals:
    void sessionCreated(const QString& sessionId);
    void sessionRemoved(const QString& sessionId);
    void sessionRestored(const QString& sessionId);
    void sessionTimeout(const QString& sessionId);

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
        QString sessionId;
        QWebSocket* socket;
        QAtomicInt nextMessageId = 0;
        QDateTime lastActivity;
        bool isActive = true;

        Session() : socket(nullptr), nextMessageId(0), lastActivity(QDateTime::currentDateTime()), isActive(true) {}
        Session(const QString& id, QWebSocket* sock)
            : sessionId(id), socket(sock), nextMessageId(0), lastActivity(QDateTime::currentDateTime()), isActive(true)
        {
        }
    };

    QMap<QString, Session*> m_sessions;

    int m_sessionCount;
    mutable QReadWriteLock m_sessionsLock;

    int m_sessionTimeoutMs = 300000;  // 5分钟超时
    QTimer* m_cleanupTimer = nullptr;
};

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_SESSIONMANAGER_H
