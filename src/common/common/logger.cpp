#include "logger.h"

#include "debug.h"

#ifdef QT_CORE_LIB
#include <QStringLiteral>
#endif

constexpr std::string_view InfoString = "[INFO] ";
constexpr std::string_view DebugString = "[DEBUG] ";
constexpr std::string_view ErrorString = "[ERROR] ";

#ifdef QT_CORE_LIB
QString INFO_STRING = QStringLiteral("[Info] ");
QString DEBUG_STRING = QStringLiteral("[Debug] ");
QString ERROR_STRING = QStringLiteral("[Error] ");

// void Logger::info(const char *s)
// {
//     qDebug().noquote() << INFO_STRING << QString(s);
// }

void Logger::info(std::ostringstream &s)
{
    qDebug().noquote() << INFO_STRING << QString(s.str().c_str());
}

void Logger::debug(std::ostringstream &s)
{
    qDebug().noquote() << DEBUG_STRING << QString(s.str().c_str());
}

void Logger::error(std::ostringstream &s)
{
    qDebug().noquote() << ERROR_STRING << QString(s.str().c_str());
}

Logger::MainLogger Logger::info()
{
    qDebug() << QString(std::string(DebugString).c_str());
    return qDebug();
}
#else
void Logger::info(std::ostringstream &s)
{
    Logger::info() << s.str() << std::endl;
}

void Logger::debug(std::ostringstream &s)
{
    Logger::debug() << s.str() << std::endl;
}

void Logger::error(std::ostringstream &s)
{
    Logger::error() << s.str() << std::endl;
}

Logger::MainLogger &Logger::info()
{
    std::clog << InfoString;
    return std::clog;
}
Logger::MainLogger &Logger::debug()
{
    std::clog << DebugString;
    return std::clog;
}
#endif

std::ostream &Logger::error()
{
    std::cerr << ErrorString;
    return std::cerr;
}

void Logger::debug(const char *s)
{
#ifdef _DEBUG
    std::clog << DebugString << s << std::endl;
#endif
}
void Logger::debug(std::string s)
{
#ifdef _DEBUG
    std::clog << DebugString << s << std::endl;
#endif
}

void Logger::info(std::string &s)
{
    std::cout << InfoString << s << std::endl;
}

void Logger::error(std::string &s)
{
    std::cerr << ErrorString << s << std::endl;
}
void Logger::error(std::string s)
{
    std::cerr << ErrorString << s << std::endl;
}
