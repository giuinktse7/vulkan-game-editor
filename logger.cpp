#include "logger.h"

#include "debug.h"

constexpr std::string_view InfoString = "[INFO] ";
constexpr std::string_view DebugString = "[DEBUG] ";
constexpr std::string_view ErrorString = "[ERROR] ";

std::ostream &Logger::info()
{
    std::cout << InfoString;
    return std::cout;
}

std::ostream &Logger::debug()
{
    std::clog << DebugString;
    return std::clog;
}

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
