#pragma once

#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>

#include "commutils.h"

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

class DateTime {
 public:
  using system_clock = std::chrono::system_clock;
  using steady_clock = std::chrono::steady_clock;

  /**
   * Returns the formatted current time as a string.
   *
   * @param fmt The format string for the time. Defaults to "%Y-%m-%d %H:%M:%S".
   * @return The formatted current time as a string.
   */
  std::string GetFmtTime(const std::string &fmt = "%Y-%m-%d %H:%M:%S") {
    auto now = system_clock::now();
    auto now_c = system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), fmt.c_str());
    return ss.str();
  }

  /**
   * Returns the current time in milliseconds since the epoch.
   *
   * @return The current time in milliseconds.
   */
  long long GetMsTime() {
    auto now = steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               now.time_since_epoch())
        .count();
  }

  /**
   * Returns the current time in microseconds since the epoch.
   *
   * @return The current time in microseconds.
   */
  long long GetUsTime() {
    auto now = steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(
               now.time_since_epoch())
        .count();
  }

  /**
   * Sleeps for the specified number of milliseconds.
   *
   * @param ms The number of milliseconds to sleep.
   */
  void SleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
};