#pragma once

#include <sys/syscall.h>
#include <unistd.h>

#include <cstdint>

class SysUtils {
 public:
  pid_t get_tid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

  bool is_little_endian() {
    const uint32_t value = 1;
    return *reinterpret_cast<const unsigned char*>(&value) == 1;
  }
};