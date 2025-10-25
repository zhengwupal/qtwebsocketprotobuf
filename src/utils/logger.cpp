#include "logger.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

namespace QtWebSocketProtobuf
{

QString Logger::extractFileName(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.fileName();
}

Logger::Logger() : m_logLevel(LogLevel::Info)
{
    m_logCallback = [this](LogLevel level, const QString& message, const QString& function, int line,
                           const QString& file) {
        QString levelStr;
        switch (level) {
        case LogLevel::Debug:
            levelStr = "DEBUG";
            break;
        case LogLevel::Info:
            levelStr = "INFO";
            break;
        case LogLevel::Warning:
            levelStr = "WARNING";
            break;
        case LogLevel::Error:
            levelStr = "ERROR";
            break;
        case LogLevel::Fatal:
            levelStr = "FATAL";
            break;
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

        // 只有当全局日志级别为Debug时才显示文件名和行号
        QString functionInfo;
        if (m_logLevel == LogLevel::Debug && !function.isEmpty()) {
            functionInfo = QString("[%1:%2@%3]").arg(function).arg(line).arg(file);
        }

        QTextStream out(stdout);
        out << QString("[%1] [%2]%3 %4\n").arg(timestamp, levelStr, functionInfo, message);
        out.flush();
    };
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
}

void Logger::setLogCallback(LogCallback callback)
{
    if (callback) {
        m_logCallback = callback;
    }
}

void Logger::log(LogLevel level, const QString& message, const QString& function, int line, const QString& file)
{
    if (level >= m_logLevel && m_logCallback) {
        m_logCallback(level, message, function, line, file);
    }
}

void Logger::debug(const QString& message, const QString& function, int line, const QString& file)
{
    log(LogLevel::Debug, message, function, line, file);
}

void Logger::info(const QString& message, const QString& function, int line, const QString& file)
{
    log(LogLevel::Info, message, function, line, file);
}

void Logger::warning(const QString& message, const QString& function, int line, const QString& file)
{
    log(LogLevel::Warning, message, function, line, file);
}

void Logger::error(const QString& message, const QString& function, int line, const QString& file)
{
    log(LogLevel::Error, message, function, line, file);
}

void Logger::fatal(const QString& message, const QString& function, int line, const QString& file)
{
    log(LogLevel::Fatal, message, function, line, file);
}

QString Logger::extractFunctionName(const QString& function)
{
    QRegularExpression re("\\b(\\w+)(?=\\()");
    QRegularExpressionMatch match = re.match(function);

    if (match.hasMatch()) {
        QString functionName = match.captured(1);

        if (functionName == "operator()") {
            QRegularExpression lambdaRe("\\[.*\\]\\s*\\(.*\\)");
            QRegularExpressionMatch lambdaMatch = lambdaRe.match(function);
            if (lambdaMatch.hasMatch()) {
                return QString("lambda%1").arg(lambdaMatch.captured(0));
            }
            return "lambda";
        }

        return functionName;
    }

    return function;
}

}  // namespace QtWebSocketProtobuf
