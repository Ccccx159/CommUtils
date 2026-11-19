#include "commutils/sys_utils_.h"
#include "gtest/gtest.h"
#include <thread>

TEST(SysUtils, EndiannessMatchesMemoryRepresentation) {
  const uint32_t value = 0x01020304;
  const unsigned char first = *reinterpret_cast<const unsigned char*>(&value);
  EXPECT_EQ(SysUtils().is_little_endian(), first == 0x04);
}

TEST(SysUtils, ThreadIdsDiffer) {
  const pid_t main_tid = SysUtils().get_tid();
  pid_t worker_tid = 0;
  std::thread worker([&] { worker_tid = SysUtils().get_tid(); });
  worker.join();
  EXPECT_NE(main_tid, worker_tid);
  EXPECT_GT(worker_tid, 0);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
