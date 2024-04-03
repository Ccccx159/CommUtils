#pragma once
#include <dlfcn.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <string>

#include "commutils.h"

#define BACKTRACE_STACK_MAX_SIZE (16)

class Backtrace {
 private:
  Backtrace() { signal(SIGSEGV, SignalHandler); }

  static void SignalHandler(int signo) {
    LOG(FATAL) << "===========>>>Catch Signal " << signo << "<<<===========";
    LOG(FATAL) << "dump stack begin...";
    [] {
      void* buf[BACKTRACE_STACK_MAX_SIZE];
      int nptr = backtrace(buf, BACKTRACE_STACK_MAX_SIZE);
      LOG(FATAL) << "backtrace() captured " << nptr << " addresses";
      char** strings = backtrace_symbols(buf, nptr);
      if (nullptr == strings) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
      }
      for (int i = 0; i < nptr; i++) {
        std::string extra_info;
        if (i == 3) {
          // 使用正则匹配找到最后 [] 内的地址
          std::string addr = strings[i];
          addr = addr.substr(addr.rfind('[') + 1,
                             addr.rfind(']') - addr.find('[') - 1);
          // 将 addr 字符串转换为地址
          void* p = (void*)std::stoul(addr, nullptr, 16);
          // 使用dladdr函数获取地址对应的符号信息
          Dl_info info;
          if (dladdr(p, &info)) {
            try {
              extra_info = fmt::format(
                  "segfault_addr:{:p}, symbol_file: {}, file_base: {:p}, "
                  "symbol_name: {}, symbol_addr: {:p}",
                  fmt::ptr(p), info.dli_fname, fmt::ptr(info.dli_fbase),
                  info.dli_sname, info.dli_saddr);
            } catch (const std::exception& e) {
              extra_info = fmt::format(
                  "segfault_addr:{:p}, symbol_file: {}, file_base: {:p}",
                  fmt::ptr(p), info.dli_fname, fmt::ptr(info.dli_fbase));
            }
          }
        }
        LOG(FATAL) << '[' << std::setw(2) << std::setfill('0') << i << "] "
                   << strings[i]  << " " << extra_info;
      }
      free(strings);
    }();
    LOG(FATAL) << "dump stack end...";
    LOG(FATAL) << "===========>>>Catch Signal " << signo << "<<<===========";
    std::string cmd("cat /proc/self/maps");
    cmd.replace(cmd.find("self"), std::string("self").length(),
                std::to_string(getpid()));
    system(cmd.c_str());
    std::cerr << "catch signal " << signo << std::endl;
    signal(signo, SIG_DFL);
    raise(signo);
  }

 public:
  static Backtrace& GetBacktraceInstance() {
    static Backtrace ins;
    return ins;
  }
};

class BacktraceInitializer {
 public:
  BacktraceInitializer() {
    // 在这里执行初始化代码
    Backtrace::GetBacktraceInstance();
    // LOG(WARN) << "Initializer  " << &bt;
  }
};

// 定义全局对象
static BacktraceInitializer backtrace_initializer;
