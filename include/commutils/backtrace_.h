#pragma once
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
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
        LOG(FATAL) << '[' << std::setw(2) << std::setfill('0') << i << "] "
                   << strings[i];
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
