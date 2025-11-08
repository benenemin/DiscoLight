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
  #include <zephyr/sys/printk.h>   // vsnprintk
  #include <cstdarg>
  #include <cstring>

  // ----- Module selection (one module for this TU) -----
  #ifndef APP_LOGGER_MODULE
    #define APP_LOGGER_MODULE app
  #endif
  #ifndef APP_LOGGER_LEVEL
    #define APP_LOGGER_LEVEL LOG_LEVEL_DBG
  #endif
  LOG_MODULE_REGISTER(APP_LOGGER_MODULE, APP_LOGGER_LEVEL);

  namespace Utils {
    // Small helper: format into a local buffer.
    template <std::size_t N>
    void vfmt_to_buf(char (&buf)[N], const char* tag,
                            const char* fmt, va_list ap) {
      int off = 0;
      // The above trick won't work; build prefix separately:
      if (tag) {
        int p = snprintk(buf, N, "[%s] ", tag);
        off = (p < 0) ? 0 : (p >= static_cast<int>(N) ? static_cast<int>(N) - 1 : p);
      }
      int r = vsnprintk(buf + off, N - off, fmt, ap);
      (void)r;
      buf[N - 1] = '\0';
    }

    template <std::size_t N>
    void make_text(char (&out)[N], const char* tag, const char* text) {
      if (text && *text) {
        snprintk(out, N, "%s", text);
      } else if (tag) {
        snprintk(out, N, "%s", tag);
      } else {
        out[0] = '\0';
      }
      out[N - 1] = '\0';
    }

    static constexpr std::size_t kBufSize = 1024;

    void Logger::debug(const char* fmt, ...) const
    {
      char buf[kBufSize];
      va_list ap; va_start(ap, fmt);
      vfmt_to_buf(buf, tag_, fmt, ap);
      va_end(ap);
      LOG_DBG("%s", buf);
    }

    void Logger::info(const char* fmt, ...) const
    {
      char buf[kBufSize];
      va_list ap; va_start(ap, fmt);
      vfmt_to_buf(buf, tag_, fmt, ap);
      va_end(ap);
      LOG_INF("%s", buf);
    }

    void Logger::warning(const char* fmt, ...) const
    {
      char buf[kBufSize];
      va_list ap; va_start(ap, fmt);
      vfmt_to_buf(buf, tag_, fmt, ap);
      va_end(ap);
      LOG_WRN("%s", buf);
    }

    void Logger::error(const char* fmt, ...) const
    {
      char buf[kBufSize];
      va_list ap; va_start(ap, fmt);
      vfmt_to_buf(buf, tag_, fmt, ap);
      va_end(ap);
      LOG_ERR("%s", buf);
    }
  } // namespace Utils

#else // ----- !APP_LOGGER_ENABLED : compile to no-ops -------------------------

  #include <cstdarg>

  namespace Utils {

  void Logger::dbg(const char*, ...) const {}
  void Logger::inf(const char*, ...) const {}
  void Logger::wrn(const char*, ...) const {}
  void Logger::err(const char*, ...) const {}

  } // namespace Utils

#endif
