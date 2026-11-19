#include "commutils/tracer_.h"
#include "gtest/gtest.h"
#include <atomic>
#include <cmath>
#include <cstdio>

class TracerTest : public testing::Test {
 protected:
  std::string path = "tracer_regression_" + std::to_string(getpid()) + ".json";
  json Read() {
    std::ifstream file(path);
    json report;
    file >> report;
    return report.at("traceEvents");
  }
  void TearDown() override { std::remove(path.c_str()); }
};

TEST_F(TracerTest, StopDrainsBurstFromMultipleThreads) {
  Tracer tracer(path);
  std::vector<std::thread> workers;
  for (int tid = 0; tid < 4; ++tid) {
    workers.emplace_back([&, tid] {
      for (int n = 0; n < 1000; ++n) {
        tracer.begin("work", {{"worker", tid}, {"iteration", n}});
        tracer.end("work");
      }
    });
  }
  for (auto& worker : workers) worker.join();
  tracer.stop();
  tracer.stop();
  tracer.begin("ignored after stop");
  const json events = Read();
  ASSERT_EQ(events.size(), 8000U);
  std::map<uint64_t, size_t> counts;
  for (const auto& event : events) {
    ++counts[event.at("tid").get<uint64_t>()];
    if (event.at("ph") == "E") {
      EXPECT_TRUE(event.at("args").count("cpu_time_us"));
      EXPECT_FALSE(event.at("args").count("unmatched_begin"));
      EXPECT_TRUE(std::isfinite(event.at("args").at("dutycycle").get<double>()));
    }
  }
  EXPECT_EQ(counts.size(), 4U);
  for (const auto& count : counts) EXPECT_EQ(count.second, 2000U);
}

TEST_F(TracerTest, SameNameNestingUsesStackAndMicroseconds) {
  Tracer tracer(path);
  tracer.begin("nested");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  tracer.begin("nested");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  tracer.end("nested");
  tracer.end("nested");
  tracer.stop();
  const json events = Read();
  ASSERT_EQ(events.size(), 4U);
  const double inner = events[2]["args"]["wall_time_us"];
  const double outer = events[3]["args"]["wall_time_us"];
  EXPECT_GT(outer, inner);
  EXPECT_GE(inner, 1000.0);
  // Nanosecond timestamps would violate this bound by three orders of magnitude.
  EXPECT_LT(inner, 5000000.0);
  EXPECT_NEAR(inner, events[2]["ts"].get<double>() - events[1]["ts"].get<double>(), .001);
  EXPECT_NEAR(outer, events[3]["ts"].get<double>() - events[0]["ts"].get<double>(), .001);
  EXPECT_NEAR(events[2]["args"]["cpu_time_us"].get<double>(),
              events[2]["tts"].get<double>() - events[1]["tts"].get<double>(), .001);
}

TEST_F(TracerTest, CpuTimeDistinguishesBusyInterfaceFromWaiting) {
  Tracer tracer(path);
  auto cpu_ns = [] {
    timespec ts{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
  };
  {
    Tracer::ScopedEvent event(tracer, "busy_interface");
    const int64_t start = cpu_ns();
    while (cpu_ns() - start < 20000000) {}
  }
  {
    Tracer::ScopedEvent event(tracer, "waiting_interface");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  tracer.stop();
  const json events = Read();
  ASSERT_EQ(events.size(), 4U);
  const double busy_cpu = events[1]["args"]["cpu_time_us"];
  const double wait_cpu = events[3]["args"]["cpu_time_us"];
  EXPECT_GE(busy_cpu, 20000.0);
  EXPECT_LT(wait_cpu, busy_cpu);
  EXPECT_LT(events[3]["args"]["dutycycle"].get<double>(), 50.0);
}

TEST_F(TracerTest, ScopeEndsOnExceptionAndDestructorFlushes) {
  {
    Tracer tracer(path);
    try {
      Tracer::ScopedEvent event(tracer, "throws", {{"request_id", 42}});
      throw std::runtime_error("test");
    } catch (const std::runtime_error&) {}
  }
  const json events = Read();
  ASSERT_EQ(events.size(), 2U);
  EXPECT_EQ(events[0]["args"]["request_id"], 42);
  EXPECT_EQ(events[1]["ph"], "E");
  EXPECT_TRUE(events[1]["args"].count("cpu_time_us"));
}

TEST_F(TracerTest, StopMayRaceWithProducersAndOtherStopCalls) {
  Tracer tracer(path);
  tracer.begin("sentinel");
  tracer.end("sentinel");
  std::atomic<bool> started(false);
  std::thread producer([&] {
    started.store(true);
    for (int i = 0; i < 1000; ++i) {
      tracer.begin("racing");
      tracer.end("racing");
    }
  });
  while (!started.load()) std::this_thread::yield();
  std::thread stop1([&] { tracer.stop(); });
  std::thread stop2([&] { tracer.stop(); });
  producer.join();
  stop1.join();
  stop2.join();
  const json events = Read();
  EXPECT_GE(events.size(), 2U);
  EXPECT_EQ(events[0]["name"], "sentinel");
  EXPECT_EQ(events[1]["name"], "sentinel");
}

TEST_F(TracerTest, InvalidArgumentsAndUnmatchedEndsAreExplicit) {
  EXPECT_THROW(Tracer("/nonexistent/commutils/trace.json"), std::runtime_error);
  Tracer tracer(path);
  EXPECT_THROW(tracer.begin("invalid", json::array()), std::invalid_argument);
  tracer.end("unmatched");
  tracer.stop();
  const json events = Read();
  ASSERT_EQ(events.size(), 1U);
  EXPECT_EQ(events[0]["args"]["unmatched_begin"], true);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
