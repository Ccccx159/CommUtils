#pragma once

#ifdef _WITH_SPDLOG
#include "spdlog/spdlog.h"

#define LOG_DEFAULT_PATTERN "[%Y-%m-%d %T.%e] [%^%l%$] [%s:%#] [%!] [t %t] %v"
#define LOG_DEFAULT_LEVEL spdlog::level::debug

#undef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#define INITIALIZE_DEFAULT_LOGGER(pattern_, level_)            \
  do {                                                         \
    if (typeid(pattern_) == typeid(std::string) &&             \
        std::string(pattern_).length() > 0) {                  \
      spdlog::set_pattern(pattern_);                           \
    } else {                                                   \
      spdlog::set_pattern(LOG_DEFAULT_PATTERN);                \
    }                                                          \
    if (typeid(level_) == typeid(spdlog::level::level_enum)) { \
      spdlog::set_level(level_);                               \
    } else {                                                   \
      spdlog::set_level(LOG_DEFAULT_LEVEL);                    \
    }                                                          \
  } while (0)

#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

#define TRACE spdlog::level::trace
#define DEBUG spdlog::level::debug
#define INFO spdlog::level::info
#define WARN spdlog::level::warn
#define WARNING spdlog::level::warn
#define ERROR spdlog::level::err
#define CRITICAL spdlog::level::critical
#define FATAL spdlog::level::critical

#define LOG(level) \
  Logger(level, __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE())

// 简单封装 spdlog，使其支持流式输出，此时仅支持 C++ stream 输出标准格式化模式
// 兼容 glog 用法
#include <sstream>
class Logger {
 public:
  Logger(spdlog::level::level_enum level, const char *file_namem,
         const char *function_name, unsigned int line)
      : level_(level),
        file_name_(file_namem),
        function_name_(function_name),
        line_(line) {}
  ~Logger() {
    spdlog::default_logger_raw()->log(
        spdlog::source_loc(file_name_, line_, function_name_), level_,
        msg_.str());
  }
  template <typename T>
  Logger &operator<<(const T &msg) {
    msg_ << msg << ' ';
    return *this;
  }

 private:
  spdlog::level::level_enum level_;
  std::ostringstream msg_;
  const char *file_name_;
  const char *function_name_;
  unsigned int line_;
};

// XXX: 如果 c++
// 标准大于等于17，可使用默认模板参数、constexpr构造函数、以及推导指引进行封装
#if __cplusplus >= 201703L
struct source_location {
  constexpr source_location(const char *file_name = __builtin_FILE(),
                            const char *function_name = __builtin_FUNCTION(),
                            unsigned int line = __builtin_LINE()) noexcept
      : file_name_(file_name), function_name_(function_name), line_(line) {}
  constexpr const char *file_name() const noexcept { return file_name_; }
  constexpr const char *function_name() const noexcept {
    return function_name_;
  }
  constexpr unsigned int line() const noexcept { return line_; }

 private:
  const char *file_name_;
  const char *function_name_;
  const unsigned int line_;
};

inline constexpr spdlog::source_loc get_log_source_location(
    const source_location &location = {}) {
  return spdlog::source_loc{location.file_name(),
                            static_cast<std::int32_t>(location.line()),
                            location.function_name()};
}

template <typename... Args>
struct info {
  constexpr info(fmt::format_string<Args...> fmt, Args &&...args,
                 spdlog::source_loc location = get_log_source_location()) {
    spdlog::default_logger_raw()->log(location, spdlog::level::info, fmt,
                                      std::forward<Args>(args)...);
  }
};

#else
// xxx
#endif  // ifdef __cplusplus >= 201703L

#else

#include <sys/syscall.h>
#include <unistd.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

// g++ 5.0 一下版本需要自行实现 std::put_time() 和 std::get_time()
#if defined(__GNUC__) && (__GNUC__ < 5)
namespace std {
template <typename _CharT>
struct _Put_time {
  const std::tm *_M_tmb;
  const _CharT *_M_fmt;
};

/**
 *  @brief  Extended manipulator for formatting time.
 *
 *  This manipulator uses time_put::put to format time.
 *  [ext.manip]
 *
 *  @param __tmb  struct tm time data to format.
 *  @param __fmt  format string.
 */
template <typename _CharT>
inline _Put_time<_CharT> put_time(const std::tm *__tmb, const _CharT *__fmt) {
  return {__tmb, __fmt};
}

template <typename _CharT, typename _Traits>
basic_ostream<_CharT, _Traits> &operator<<(basic_ostream<_CharT, _Traits> &__os,
                                           _Put_time<_CharT> __f) {
  typename basic_ostream<_CharT, _Traits>::sentry __cerb(__os);
  if (__cerb) {
    ios_base::iostate __err = ios_base::goodbit;
    __try {
      typedef ostreambuf_iterator<_CharT, _Traits> _Iter;
      typedef time_put<_CharT, _Iter> _TimePut;

      const _CharT *const __fmt_end = __f._M_fmt + _Traits::length(__f._M_fmt);

      const _TimePut &__mp = use_facet<_TimePut>(__os.getloc());
      if (__mp.put(_Iter(__os.rdbuf()), __os, __os.fill(), __f._M_tmb,
                   __f._M_fmt, __fmt_end)
              .failed())
        __err |= ios_base::badbit;
    }
    __catch(__cxxabiv1::__forced_unwind &) {
      __os._M_setstate(ios_base::badbit);
      __throw_exception_again;
    }
    __catch(...) { __os._M_setstate(ios_base::badbit); }
    if (__err) __os.setstate(__err);
  }
  return __os;
}
template <typename _CharT>
struct _Get_time {
  std::tm *_M_tmb;
  const _CharT *_M_fmt;
};

/**
 *  @brief  Extended manipulator for extracting time.
 *
 *  This manipulator uses time_get::get to extract time.
 *  [ext.manip]
 *
 *  @param __tmb  struct to extract the time data to.
 *  @param __fmt  format string.
 */
template <typename _CharT>
inline _Get_time<_CharT> get_time(std::tm *__tmb, const _CharT *__fmt) {
  return {__tmb, __fmt};
}

template <typename _CharT, typename _Traits>
basic_istream<_CharT, _Traits> &operator>>(basic_istream<_CharT, _Traits> &__is,
                                           _Get_time<_CharT> __f) {
  typename basic_istream<_CharT, _Traits>::sentry __cerb(__is, false);
  if (__cerb) {
    ios_base::iostate __err = ios_base::goodbit;
    __try {
      typedef istreambuf_iterator<_CharT, _Traits> _Iter;
      typedef time_get<_CharT, _Iter> _TimeGet;

      const _CharT *const __fmt_end = __f._M_fmt + _Traits::length(__f._M_fmt);

      const _TimeGet &__mg = use_facet<_TimeGet>(__is.getloc());
      __mg.get(_Iter(__is.rdbuf()), _Iter(), __is, __err, __f._M_tmb,
               __f._M_fmt, __fmt_end);
    }
    __catch(__cxxabiv1::__forced_unwind &) {
      __is._M_setstate(ios_base::badbit);
      __throw_exception_again;
    }
    __catch(...) { __is._M_setstate(ios_base::badbit); }
    if (__err) __is.setstate(__err);
  }
  return __is;
}
}  // namespace std
#endif


#define TRACE "trace"
#define DEBUG "debug"
#define INFO "info"
#define WARN "warning"
#define WARNING "warning"
#define ERROR "error"
#define CRITICAL "critical"
#define FATAL "critical"
#define RESET "reset"

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#include "fmt/format.h"

#define LOG(level) \
  Logger(level, __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE())

// 简单封装 spdlog，使其支持流式输出，此时仅支持 C++ stream 输出标准格式化模式
// 兼容 glog 用法
class Logger {
 public:
  Logger(std::string level, const char *file_namem, const char *function_name,
         unsigned int line)
      : level_(level),
        file_name_(file_namem),
        function_name_(function_name),
        line_(line) {}
  ~Logger() {
    using namespace std::chrono;
    auto now = system_clock::to_time_t(system_clock::now());
    std::cout << "[" << std::put_time(localtime(&now), "%F %T") << "] ["
              << logger_color_map[level_] << level_ << logger_color_map[RESET]
              << "] ["
              << std::string(file_name_)
                     .substr(std::string(file_name_).find_last_of('/') + 1)
              << ":" << line_ << "] [" << function_name_
              << "] [t:" << static_cast<pid_t>(::syscall(SYS_gettid)) << "]"
              << msg_.str() << std::endl;
  }
  template <typename T>
  Logger &operator<<(const T &msg) {
    msg_ << ' ' << msg;
    return *this;
  }

 private:
  std::unordered_map<std::string, std::string> logger_color_map = {
      {TRACE, "\033[1;37m"},       {DEBUG, "\033[1;34m"},
      {INFO, "\033[1;32m"},        {WARN, "\033[1;33m"},
      {WARNING, "\033[1;33m"},     {ERROR, "\033[1;31m"},
      {CRITICAL, "\033[1;41;37m"}, {RESET, "\033[0m"},
  };

 private:
  std::string level_;
  std::ostringstream msg_;
  const char *file_name_;
  const char *function_name_;
  unsigned int line_;
};

template <typename... Args>
void Logger_fmt(std::string const &level, const char *file_name,
                const char *function_name, unsigned int line,
                fmt::format_string<Args...> fmt, Args &&...args) {
  using namespace std::chrono;
  auto now = system_clock::to_time_t(system_clock::now());
  std::unordered_map<std::string, std::string> logger_color_map = {
      {TRACE, "\033[1;37m"},       {DEBUG, "\033[1;34m"},
      {INFO, "\033[1;32m"},        {WARN, "\033[1;33m"},
      {WARNING, "\033[1;33m"},     {ERROR, "\033[1;31m"},
      {CRITICAL, "\033[1;41;37m"}, {RESET, "\033[0m"},
  };
  std::cout << "[" << std::put_time(localtime(&now), "%F %T") << "] ["
            << logger_color_map[level] << level
            << logger_color_map[RESET] << "] ["
            << std::string(file_name).substr(
                   std::string(file_name).find_last_of('/') + 1)
            << ":" << line << "] [" << function_name
            << "] [t:" << static_cast<pid_t>(::syscall(SYS_gettid)) << "] "
            << fmt::format(fmt, std::forward<Args>(args)...) << std::endl;
}

#define LOG_INFO(...)                                                        \
  Logger_fmt(INFO, __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), \
             __VA_ARGS__)

#define LOG_DEBUG(...)                                                        \
  Logger_fmt(DEBUG, __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), \
             __VA_ARGS__)

#define LOG_WARN(...)                                                        \
  Logger_fmt(WARN, __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), \
             __VA_ARGS__)

#define LOG_ERROR(...)                                                        \
  Logger_fmt(ERROR, __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), \
             __VA_ARGS__)

#define LOG_CRITICAL(...)                                      \
  Logger_fmt(CRITICAL, __builtin_FILE(), __builtin_FUNCTION(), \
             __builtin_LINE(), __VA_ARGS__)

#define LOG_FATAL(...) LOG_CRITICAL(__VA_ARGS__)
#define LOG_WARNING(...) LOG_WARN(__VA_ARGS__)

#undef LOG_DEFAULT_PATTERN
#undef LOG_DEFAULT_LEVEL
#define INITIALIZE_DEFAULT_LOGGER(...)                                         \
  do {                                                                         \
    LOG(WARN) << "Always use DEBUG level and default pattern, initialization " \
                 "is invalid!";                                                \
  } while (0)

#endif
