#include "commutils/commutils.h"
#include "gtest/gtest.h"
#include "test.h"

TEST(Backtrace, CatchSegmentFaultSig) {
  // 使用 volatile 和 char* 来规避 UBSan 的检测
  // 这样能真正触发 SIGSEGV 而不是被 UBSan 中止
  EXPECT_DEATH([]{ volatile char* p = (char*)0x1; *p = 'x'; }(), "catch signal " + std::to_string(SIGSEGV));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}