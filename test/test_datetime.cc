#include "commutils/commutils.h"
#include "gtest/gtest.h"

class DateTimeTest : public ::testing::Test {
 protected:
  DateTime dt;
};

TEST_F(DateTimeTest, GetFmtTime) {
  std::string time = dt.GetFmtTime();
  EXPECT_NE(time.find("-"), std::string::npos);
  EXPECT_NE(time.find(":"), std::string::npos);
}

TEST_F(DateTimeTest, GetMsTime) {
  auto start = dt.GetMsTime();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  auto end = dt.GetMsTime();
  EXPECT_GE(end - start, 100);
}

TEST_F(DateTimeTest, GetUsTime) {
  auto start = dt.GetUsTime();
  std::this_thread::sleep_for(std::chrono::microseconds(100));
  auto end = dt.GetUsTime();
  EXPECT_GE(end - start, 100);
}

TEST_F(DateTimeTest, SleepMs) {
  auto start = dt.GetMsTime();
  dt.SleepMs(100);
  auto end = dt.GetMsTime();
  EXPECT_GE(end - start, 100);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}