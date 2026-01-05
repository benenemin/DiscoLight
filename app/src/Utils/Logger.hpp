//
// Created by bened on 22/10/2025.
//

#pragma once
#include <array>
#include <cstddef>

#include "zephyr/kernel.h"
#include "zephyr/sys/printk.h"

namespace Utils {

    /**
     * C++ wrapper around Zephyr logging that keeps LOG_* in the .cpp only.
     * In non-debug builds, all methods are compiled to no-ops.
     *
     * Build switches:
     *   - Define APP_LOGGER_ENABLED=1 to force-enable (typically in Debug)
     *   - Or rely on default: enabled when CONFIG_LOG && !NDEBUG
     *
     * Module selection:
     *   - Change module name/level at compile time:
     *       -DAPP_LOGGER_MODULE=myapp -DAPP_LOGGER_LEVEL=LOG_LEVEL_INF
     */
    class Logger {
    public:
        explicit Logger(const char* tag = nullptr)  : tag_(tag) {}

        // printf-style APIs (thread context). Safe: formats into a fixed buffer.
        void debug(const char* fmt, ...) const;
        void info(const char* fmt, ...) const;
        void warning(const char* fmt, ...) const;
        void error(const char* fmt, ...) const;

    private:
        const char* tag_{nullptr};
    };

} // namespace util

