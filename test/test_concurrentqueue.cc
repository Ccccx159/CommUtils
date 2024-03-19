#include "commutils/commutils.h"
#include "test.h"
#include "gtest/gtest.h"

#ifndef MY_BLOCK_SIZE
#define MY_BLOCK_SIZE 8
#endif

struct MyTraits: public moodycamel::ConcurrentQueueDefaultTraits {
  static const size_t BLOCK_SIZE = MY_BLOCK_SIZE;
};

using ccque_int = moodycamel::ConcurrentQueue<int, MyTraits>;

TEST(ConcurrentQueue, Basic_1) {
  ccque_int q(1);
  for (int i = 0; i < MY_BLOCK_SIZE; i++) {
    EXPECT_EQ(true, q.try_enqueue(i));
  }
  EXPECT_EQ(false, q.try_enqueue(MY_BLOCK_SIZE));
  int x = 0;
  for (int i = 0; i < MY_BLOCK_SIZE; i++) {
    EXPECT_EQ(true, q.try_dequeue(x));
    EXPECT_EQ(i, x);
  }
  EXPECT_EQ(false, q.try_dequeue(x));  
}

TEST(ConcurrentQueue, Basic_7) {
  ccque_int q(MY_BLOCK_SIZE - 1);
  for (int i = 0; i < MY_BLOCK_SIZE; i++) {
    EXPECT_EQ(true, q.try_enqueue(i));
  }
  EXPECT_EQ(false, q.try_enqueue(MY_BLOCK_SIZE));
  int x = 0;
  for (int i = 0; i < MY_BLOCK_SIZE; i++) {
    EXPECT_EQ(true, q.try_dequeue(x));
    EXPECT_EQ(i, x);
  }
  EXPECT_EQ(false, q.try_dequeue(x));  
}

TEST(ConcurrentQueue, Basic_11) {
  ccque_int q(MY_BLOCK_SIZE + 1);
  for (int i = 0; i < MY_BLOCK_SIZE; i++) {
    EXPECT_EQ(true, q.try_enqueue(i));
  }
  for (int i = 0; i < MY_BLOCK_SIZE; i++) {
    EXPECT_EQ(true, q.try_enqueue(i + MY_BLOCK_SIZE));
  }
  EXPECT_EQ(false, q.try_enqueue(2 * MY_BLOCK_SIZE + 1));
  int x = 0;
  for (int i = 0; i < 2*MY_BLOCK_SIZE; i++) {
    EXPECT_EQ(true, q.try_dequeue(x));
    EXPECT_EQ(i, x);
  }
  EXPECT_EQ(false, q.try_dequeue(x));
}

TEST(ConcurrentQueue, Stress) {
  ccque_int q;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; i++) {
    threads.push_back(std::thread([&q, i] {
      for (int j = 0; j < 1000; j++) {
        EXPECT_EQ(true, q.enqueue(i * 1000 + j));
      }
    }));
  }
  for (auto& t : threads) {
    t.join();
  }
  std::vector<int> v;
  int x;
  while (q.try_dequeue(x)) {
    v.push_back(x);
  }
  std::sort(v.begin(), v.end());
  for (int i = 0; i < 1000; i++) {
    // LOG(ERROR) << fmt::format("index: {}, value: {}", i, v[i]);
    EXPECT_EQ(i, v[i]);
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}