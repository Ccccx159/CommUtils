#pragma once

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#define BACKTRACE_STACK_MAX_SIZE (16)

class Backtrace {
 public:
  // Explicit opt-in: merely including this header never changes signal handlers.
  static Backtrace& GetBacktraceInstance() {
    static Backtrace ins;
    return ins;
  }

  // Full stack unwinding is only available in normal execution context.
  // For fatal faults use the fault address below and the OS core dump.
  static void DumpStack(int fd = STDERR_FILENO) {
    void* frames[BACKTRACE_STACK_MAX_SIZE];
    const int count = backtrace(frames, BACKTRACE_STACK_MAX_SIZE);
    backtrace_symbols_fd(frames, count, fd);
  }

 private:
  Backtrace() {
    struct sigaction action{};
    action.sa_sigaction = SignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    if (sigaction(SIGSEGV, &action, nullptr) != 0)
      throw std::runtime_error("Failed to install SIGSEGV handler");
  }

  static void SignalHandler(int signo, siginfo_t* info, void*) {
    // Only stack buffers and async-signal-safe calls; no logger, heap or shell.
    static const char message[] = "catch signal SIGSEGV, fault address: 0x";
    write(STDERR_FILENO, message, sizeof(message) - 1);
    const uintptr_t addr = reinterpret_cast<uintptr_t>(info->si_addr);
    char hex[sizeof(uintptr_t) * 2 + 1];
    for (size_t i = 0; i < sizeof(uintptr_t) * 2; ++i) {
      const unsigned shift = static_cast<unsigned>((sizeof(uintptr_t) * 2 - 1 - i) * 4);
      hex[i] = "0123456789abcdef"[(addr >> shift) & 0xf];
    }
    hex[sizeof(hex) - 1] = '\n';
    write(STDERR_FILENO, hex, sizeof(hex));
    // SA_RESETHAND restores the default disposition before entry. The pending
    // signal terminates the process (and permits a core dump) on handler return.
    raise(signo);
  }
};

// Retained for callers who explicitly want an initializer object.
class BacktraceInitializer {
 public:
  BacktraceInitializer() { Backtrace::GetBacktraceInstance(); }
};
