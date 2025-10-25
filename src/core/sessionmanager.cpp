#include "sessionmanager.h"
#include "../utils/logger.h"
#include "broadcastresult.h"
#include <QDateTime>
#include <QDebug>
#include <QReadLocker>
#include <QUuid>
#include <QWebSocket>
#include <QWriteLocker>

namespace QtWebSocketProtobuf
{

SessionManager::SessionManager(QObject* parent) : QObject(parent), m_sessionCount(0)
{
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(30000);
    connect(m_cleanupTimer, &QTimer::timeout, this, &SessionManager::cleanupExpiredSessions);
    m_cleanupTimer->start();
}

SessionManager::~SessionManager()
{
    QWriteLocker locker(&m_sessionsLock);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        delete it.value();
    }
    m_sessions.clear();
    m_sessionCount = 0;
}

QString SessionManager::createSession(QWebSocket* socket)
{
    if (!socket) {
        LOG_ERROR("Cannot create session with null socket");
        return QString();
    }

    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Session* session = new Session(sessionId, socket);

    {
        QWriteLocker locker(&m_sessionsLock);
        m_sessions[sessionId] = session;
        m_sessionCount++;
    }

    LOG_DEBUG(QString("Created new session %1").arg(sessionId));
    emit sessionCreated(sessionId);

    return sessionId;
}

void SessionManager::removeSession(const QString& sessionId)
{
    Session* session = nullptr;
    {
        QReadLocker locker(&m_sessionsLock);
        session = m_sessions.value(sessionId);
    }

    if (!session) {
        LOG_WARNING(QString("Session %1 not found for removal").arg(sessionId));
        return;
    }

    {
        QWriteLocker locker(&m_sessionsLock);
        m_sessions.remove(sessionId);
        m_sessionCount--;
    }

    delete session;

    LOG_DEBUG(QString("Removed session %1").arg(sessionId));
    emit sessionRemoved(sessionId);
}

void SessionManager::removeSession(QWebSocket* socket)
{
    Session* session = nullptr;
    {
        QReadLocker locker(&m_sessionsLock);
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if (it.value()->socket == socket && it.value()->isActive) {
                session = it.value();
                break;
            }
        }
    }

    if (!session) {
        return;
    }

    QString sessionId = session->sessionId;
    {
        QWriteLocker locker(&m_sessionsLock);
        session->socket = nullptr;
        session->isActive = false;
        m_sessionCount--;
        LOG_DEBUG(QString("Session %1 marked as inactive for potential restore").arg(sessionId));
    }
}

void SessionManager::disconnectSession(const QString& sessionId, const QString& reason)
{
    QWebSocket* socket = getSocket(sessionId);
    if (!socket) {
        LOG_WARNING(QString("Session %1 not found for disconnection").arg(sessionId));
        return;
    }

    socket->close(QWebSocketProtocol::CloseCodeNormal, reason);
}

QWebSocket* SessionManager::getSocket(const QString& sessionId) const
{
    QReadLocker locker(&m_sessionsLock);
    Session* session = m_sessions.value(sessionId);
    return (session && session->isActive) ? session->socket : nullptr;
}

QString SessionManager::getSessionId(QWebSocket* socket) const
{
    QReadLocker locker(&m_sessionsLock);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value()->socket == socket && it.value()->isActive) {
            return it.value()->sessionId;
        }
    }
    return QString();
}

int SessionManager::sessionCount() const
{
    QReadLocker locker(&m_sessionsLock);
    return m_sessionCount;
}

bool SessionManager::hasSession(const QString& sessionId) const
{
    QReadLocker locker(&m_sessionsLock);
    Session* session = m_sessions.value(sessionId);
    return session != nullptr;
}

bool SessionManager::hasSession(QWebSocket* socket) const
{
    QReadLocker locker(&m_sessionsLock);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value()->socket == socket && it.value()->isActive) {
            return true;
        }
    }
    return false;
}

QStringList SessionManager::getAllSessionIds() const
{
    QReadLocker locker(&m_sessionsLock);
    return m_sessions.keys();
}

qint64 SessionManager::sendBinaryMessage(const QString& sessionId, const QByteArray& data)
{
    QWebSocket* socket = getSocket(sessionId);
    if (!socket) {
        LOG_WARNING(QString("Session %1 not found").arg(sessionId));
        return -1;
    }

    return socket->sendBinaryMessage(data);
}

BroadcastResult SessionManager::broadcastBinaryMessage(const QByteArray& data)
{
    QStringList sessionIds;
    {
        QReadLocker locker(&m_sessionsLock);
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if (it.value()->isActive) {
                sessionIds.append(it.value()->sessionId);
            }
        }
    }

    qint64 totalBytesSent = 0;
    int successCount = 0;
    int totalCount = sessionIds.size();
    QStringList failedSessions;
    QString error;

    for (const auto& sessionId : qAsConst(sessionIds)) {
        QWebSocket* socket = getSocket(sessionId);
        if (socket) {
            qint64 bytesSent = socket->sendBinaryMessage(data);
            if (bytesSent > 0) {
                totalBytesSent += bytesSent;
                successCount++;
            } else {
                failedSessions.append(sessionId);
            }
        } else {
            failedSessions.append(sessionId);
        }
    }

    if (totalCount == 0) {
        error = "No active sessions";
    } else if (successCount == 0) {
        error = "All sessions failed to receive broadcast message";
    }

    return BroadcastResult(totalBytesSent, successCount, totalCount, failedSessions, error);
}

quint32 SessionManager::getNextMessageId(const QString& sessionId)
{
    Session* session = nullptr;
    {
        QReadLocker locker(&m_sessionsLock);
        session = m_sessions.value(sessionId, nullptr);
    }

    if (!session) {
        LOG_WARNING(QString("Session %1 not found for getting next message ID").arg(sessionId));
        return 0;
    }

    return session->nextMessageId.fetchAndAddAcquire(1) + 1;
}

bool SessionManager::restoreSession(QWebSocket* socket, const QString& sessionId)
{
    if (!socket) {
        LOG_ERROR("Cannot restore session with null socket");
        return false;
    }

    bool restored = false;
    {
        QWriteLocker locker(&m_sessionsLock);
        Session* session = m_sessions.value(sessionId);
        if (session && !session->isActive) {
            session->socket = socket;
            session->isActive = true;
            session->lastActivity = QDateTime::currentDateTime();
            m_sessionCount++;
            restored = true;
        }
    }

    if (restored) {
        LOG_DEBUG(QString("Restored session %1").arg(sessionId));
        emit sessionRestored(sessionId);
        return true;
    } else {
        LOG_DEBUG(QString("Session %1 not found for restore or already active").arg(sessionId));
        return false;
    }
}

void SessionManager::setSessionTimeout(int timeoutMs)
{
    m_sessionTimeoutMs = timeoutMs;
    LOG_DEBUG(QString("Session timeout set to %1 ms").arg(timeoutMs));
}

int SessionManager::sessionTimeout() const
{
    return m_sessionTimeoutMs;
}

void SessionManager::updateSessionActivity(const QString& sessionId)
{
    QWriteLocker locker(&m_sessionsLock);
    Session* session = m_sessions.value(sessionId);
    if (session && session->isActive) {
        session->lastActivity = QDateTime::currentDateTime();
    }
}

void SessionManager::cleanupExpiredSessions()
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList expiredSessions;

    {
        QWriteLocker locker(&m_sessionsLock);
        for (auto it = m_sessions.begin(); it != m_sessions.end();) {
            Session* session = it.value();
            if (!session->isActive && session->socket == nullptr &&
                session->lastActivity.msecsTo(now) > m_sessionTimeoutMs) {
                expiredSessions.append(session->sessionId);
                delete session;
                it = m_sessions.erase(it);
                m_sessionCount--;
            } else {
                ++it;
            }
        }
    }

    for (const QString& sessionId : expiredSessions) {
        LOG_DEBUG(QString("Session %1 expired and removed").arg(sessionId));
        emit sessionTimeout(sessionId);
    }
}

}  // namespace QtWebSocketProtobuf
