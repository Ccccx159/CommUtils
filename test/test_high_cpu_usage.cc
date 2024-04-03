#include "commutils/commutils.h"
#include "gtest/gtest.h"

int CpuUsageSimulate() {
  long long total_time = 100, run_time = 50;
  int run_cnt = 100;
  while (run_cnt > 0) {
    auto start = DateTime().GetMsTime();
    while (DateTime().GetMsTime() - start <= run_time) {};
    DateTime().SleepMs(total_time - run_time);
    run_cnt--;
  }
  return 0;
}

TEST(HighCpuUsageSimulate, 50_percent) {
  GuestAgent::GetGuestAgentInstance();
  CpuUsageSimulate();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}