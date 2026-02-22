#include "Utils/Logger.hpp"

// ----- Enable/disable switch -----
// Default: enable when Zephyr logging is on and NDEBUG is NOT defined,
// or when APP_LOGGER_ENABLED is manually defined.
#if !defined(APP_LOGGER_ENABLED)
#if defined(CONFIG_LOG) && !defined(NDEBUG)
#define APP_LOGGER_ENABLED 1
#else
#define APP_LOGGER_ENABLED 0
#endif
#endif

#if APP_LOGGER_ENABLED

#include <zephyr/logging/log.h>

#include <cstdarg>
#include <cstdio>

// ----- Module selection (one module for this TU) -----
#ifndef APP_LOGGER_MODULE
#define APP_LOGGER_MODULE app
#endif
#ifndef APP_LOGGER_LEVEL
#define APP_LOGGER_LEVEL LOG_LEVEL_DBG
#endif
LOG_MODULE_REGISTER(APP_LOGGER_MODULE, APP_LOGGER_LEVEL);

namespace Utils
{
namespace
{
template <std::size_t N>
void VfmtToBuf(char (&buf)[N], const char* tag, const char* fmt, va_list ap)
{
    int offset = 0;
    if (tag != nullptr)
    {
        const int prefix_result = std::snprintf(buf, N, "[%s] ", tag);
        offset = (prefix_result < 0)
                     ? 0
                     : (prefix_result >= static_cast<int>(N) ? static_cast<int>(N) - 1 : prefix_result);
    }

    std::vsnprintf(buf + offset, N - offset, fmt, ap);
    buf[N - 1] = '\0';
}

constexpr std::size_t kBufSize = 1024;
} // namespace

void Logger::debug(const char* fmt, ...) const
{
    char buf[kBufSize];
    va_list ap;
    va_start(ap, fmt);
    VfmtToBuf(buf, tag_, fmt, ap);
    va_end(ap);
    LOG_DBG("%s", buf);
}

void Logger::info(const char* fmt, ...) const
{
    char buf[kBufSize];
    va_list ap;
    va_start(ap, fmt);
    VfmtToBuf(buf, tag_, fmt, ap);
    va_end(ap);
    LOG_INF("%s", buf);
}

void Logger::warning(const char* fmt, ...) const
{
    char buf[kBufSize];
    va_list ap;
    va_start(ap, fmt);
    VfmtToBuf(buf, tag_, fmt, ap);
    va_end(ap);
    LOG_WRN("%s", buf);
}

void Logger::error(const char* fmt, ...) const
{
    char buf[kBufSize];
    va_list ap;
    va_start(ap, fmt);
    VfmtToBuf(buf, tag_, fmt, ap);
    va_end(ap);
    LOG_ERR("%s", buf);
}
} // namespace Utils

#else // ----- !APP_LOGGER_ENABLED : compile to no-ops -------------------------

namespace Utils
{
void Logger::debug(const char*, ...) const {}
void Logger::info(const char*, ...) const {}
void Logger::warning(const char*, ...) const {}
void Logger::error(const char*, ...) const {}
} // namespace Utils

#endif
