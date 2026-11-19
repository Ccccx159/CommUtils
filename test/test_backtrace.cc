#include <signal.h>
namespace {
struct SignalSnapshot {
  struct sigaction action{};
  SignalSnapshot() { sigaction(SIGSEGV, nullptr, &action); }
} before_include;
}
#include "commutils/commutils.h"
#include "gtest/gtest.h"
#include "test.h"

TEST(Backtrace, HeaderDoesNotInstallSignalHandler) {
  struct sigaction current{};
  ASSERT_EQ(sigaction(SIGSEGV, nullptr, &current), 0);
  EXPECT_EQ(current.sa_handler, before_include.action.sa_handler);
}

TEST(Backtrace, StackCanBeDumpedInNormalContext) {
  int fds[2];
  ASSERT_EQ(pipe(fds), 0);
  Backtrace::DumpStack(fds[1]);
  close(fds[1]);
  char output[4096];
  EXPECT_GT(read(fds[0], output, sizeof(output)), 0);
  close(fds[0]);
}

TEST(Backtrace, CatchSegmentFaultSig) {
  Backtrace::GetBacktraceInstance();
  // 使用 volatile 和 char* 来规避 UBSan 的检测
  // 这样能真正触发 SIGSEGV 而不是被 UBSan 中止
  EXPECT_DEATH([]{ volatile char* p = (char*)0x1; *p = 'x'; }(), "catch signal SIGSEGV, fault address: 0x");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}