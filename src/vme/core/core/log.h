#include <spdlog/spdlog.h>

namespace console
{
    template <typename T>
    inline void log(const T &msg)
    {
        spdlog::info(msg);
    }

    template <typename T>
    inline void debug(const T &msg)
    {
        spdlog::debug(msg);
    }

    template <typename... Args>
    inline void log(fmt::format_string<Args...> message, Args &&...args)
    {
        spdlog::info(message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void error(fmt::format_string<Args...> message, Args &&...args)
    {
        spdlog::error(message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void debug(fmt::format_string<Args...> message, Args &&...args)
    {
        spdlog::debug(message, std::forward<Args>(args)...);
    }
}; // namespace console