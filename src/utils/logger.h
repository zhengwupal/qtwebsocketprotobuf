#ifndef QTWEBSOCKETPROTOBUF_LOGGER_H
#define QTWEBSOCKETPROTOBUF_LOGGER_H

#include "../qtwebsocketprotobufglobal.h"
#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <functional>

namespace QtWebSocketProtobuf
{

/**
 * @brief 日志级别枚举
 */
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * @brief 日志回调函数类型定义
 */
using LogCallback =
    std::function<void(LogLevel level, const QString& message, const QString& function, int line, const QString& file)>;

/**
 * @brief 简单的日志工具类
 */
class QTWEBSOCKETPROTOBUF_EXPORT Logger
{
public:
    static Logger& instance();
    void setLogLevel(LogLevel level);
    void setLogCallback(LogCallback callback);

    static QString extractFunctionName(const QString& function);
    static QString extractFileName(const QString& filePath);

    void debug(const QString& message, const QString& function = QString(), int line = 0,
               const QString& file = QString());
    void info(const QString& message, const QString& function = QString(), int line = 0,
              const QString& file = QString());
    void warning(const QString& message, const QString& function = QString(), int line = 0,
                 const QString& file = QString());
    void error(const QString& message, const QString& function = QString(), int line = 0,
               const QString& file = QString());
    void fatal(const QString& message, const QString& function = QString(), int line = 0,
               const QString& file = QString());

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const QString& message, const QString& function, int line, const QString& file);

private:
    LogLevel m_logLevel;
    LogCallback m_logCallback;
};

#define LOG_DEBUG(msg)                                                                                                 \
    QtWebSocketProtobuf::Logger::instance().debug(                                                                     \
        msg, QtWebSocketProtobuf::Logger::extractFunctionName(__PRETTY_FUNCTION__), __LINE__,                          \
        QtWebSocketProtobuf::Logger::extractFileName(__FILE__))
#define LOG_INFO(msg)                                                                                                  \
    QtWebSocketProtobuf::Logger::instance().info(                                                                      \
        msg, QtWebSocketProtobuf::Logger::extractFunctionName(__PRETTY_FUNCTION__), __LINE__,                          \
        QtWebSocketProtobuf::Logger::extractFileName(__FILE__))
#define LOG_WARNING(msg)                                                                                               \
    QtWebSocketProtobuf::Logger::instance().warning(                                                                   \
        msg, QtWebSocketProtobuf::Logger::extractFunctionName(__PRETTY_FUNCTION__), __LINE__,                          \
        QtWebSocketProtobuf::Logger::extractFileName(__FILE__))
#define LOG_ERROR(msg)                                                                                                 \
    QtWebSocketProtobuf::Logger::instance().error(                                                                     \
        msg, QtWebSocketProtobuf::Logger::extractFunctionName(__PRETTY_FUNCTION__), __LINE__,                          \
        QtWebSocketProtobuf::Logger::extractFileName(__FILE__))
#define LOG_FATAL(msg)                                                                                                 \
    QtWebSocketProtobuf::Logger::instance().fatal(                                                                     \
        msg, QtWebSocketProtobuf::Logger::extractFunctionName(__PRETTY_FUNCTION__), __LINE__,                          \
        QtWebSocketProtobuf::Logger::extractFileName(__FILE__))

}  // namespace QtWebSocketProtobuf

#endif  // QTWEBSOCKETPROTOBUF_LOGGER_H
