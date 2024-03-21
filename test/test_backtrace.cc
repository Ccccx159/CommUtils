#include "commutils/commutils.h"
#include "gtest/gtest.h"
#include "test.h"

TEST(Backtrace, CatchSegmentFaultSig) {
  EXPECT_DEATH([]{int *p = nullptr; *p = 1;}(), "catch signal " + std::to_string(SIGSEGV));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}