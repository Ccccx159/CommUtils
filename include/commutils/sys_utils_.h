#pragma once

#include <sys/syscall.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "commutils.h"

class SysUtils {
 public:
  pid_t get_tid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

  bool is_little_endian() {
    // uint32_t i = 0x12345678;
    // return (*((uint8_t*)&((uint32_t)0x12345678)) == 0x78);
    return (htonl((uint32_t)0x12345678) == (uint32_t)0x12345678);
  }
};