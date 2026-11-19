#include "commutils/guest_agent_.h"
#include "gtest/gtest.h"
#include <sys/prctl.h>
#include <cstdlib>

namespace {
std::string Sample(GuestAgent& agent) {
  for (int i = 0; i < 200; ++i) {
    const std::string text = agent.ReadAgentFile();
    if (std::count(text.begin(), text.end(), '\n') >= 2) return text;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return "";
}
}

TEST(GuestAgentLifecycle, StopInterruptsLongIntervalAndFileRemainsReadable) {
  const std::string path = "lifecycle_stat.csv";
  const auto start = std::chrono::steady_clock::now();
  {
    GuestAgent agent(GuestAgentExternalInfo(path, nullptr, nullptr, 3600, "stat"));
    EXPECT_FALSE(Sample(agent).empty());
    agent.Stop();
    agent.Stop();
    EXPECT_FALSE(agent.ReadAgentFile().empty());
  }
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
  std::remove(path.c_str());
}

TEST(GuestAgentLifecycle, ExternalCallbackGetsUserDataAndExitsBeforeDestruction) {
  std::atomic<bool> started(false), exited(false);
  int user_data = 42;
  auto callback = [&](void* arg) -> void* {
    auto* info = static_cast<GuestAgentExternalInfo*>(arg);
    EXPECT_EQ(info->arg_, &user_data);
    started.store(true);
    while (!info->WaitForStop(3600000)) {}
    exited.store(true);
    return nullptr;
  };
  {
    GuestAgent agent(GuestAgentExternalInfo("", callback, &user_data));
    for (int i = 0; i < 200 && !started.load(); ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_TRUE(started.load());
  }
  EXPECT_TRUE(exited.load());
}

TEST(GuestAgentLifecycle, StatHandlesNamesWithSpacesAndParentheses) {
  char old_name[16] = {};
  ASSERT_EQ(prctl(PR_GET_NAME, old_name), 0);
  ASSERT_EQ(prctl(PR_SET_NAME, "worker (busy)"), 0);
  const std::string path = "lifecycle_name.csv";
  {
    GuestAgent agent(GuestAgentExternalInfo(path, nullptr, nullptr, 1, "stat"));
    const std::string data = Sample(agent);
    EXPECT_NE(data.find("worker (busy)"), std::string::npos);
    EXPECT_NE(data.find('%'), std::string::npos);
    EXPECT_EQ(data.find("nan"), std::string::npos);
    EXPECT_EQ(data.find("inf"), std::string::npos);
  }
  EXPECT_EQ(prctl(PR_SET_NAME, old_name), 0);
  std::remove(path.c_str());
}

TEST(GuestAgentLifecycle, RejectsInvalidOptionsAndContainsCallbackExceptions) {
  EXPECT_THROW(GuestAgent(GuestAgentExternalInfo("", nullptr, nullptr, 0)), std::invalid_argument);
  EXPECT_THROW(GuestAgent(GuestAgentExternalInfo("", nullptr, nullptr, 1, "bad")), std::invalid_argument);
  EXPECT_THROW(GuestAgent(GuestAgentExternalInfo("/nonexistent/commutils/file")), std::runtime_error);
  GuestAgent agent(GuestAgentExternalInfo("", [](void*) -> void* {
    throw std::runtime_error("callback test error");
  }));
  agent.Stop();
}

TEST(GuestAgentLifecycle, TopChecksHeaderPidAndColumnBounds) {
  char pattern[] = "/tmp/commutils_top_XXXXXX";
  char* temp = mkdtemp(pattern);
  ASSERT_NE(temp, nullptr);
  const std::string dir(temp), script = dir + "/top";
  const char* old_path = std::getenv("PATH");
  const bool had_path = old_path != nullptr;
  const std::string saved_path = old_path ? old_path : "";
  ASSERT_EQ(setenv("PATH", dir.c_str(), 1), 0);
  const std::string pid = std::to_string(getpid());
  const std::vector<std::pair<std::string, std::string>> cases = {
    {"PID USER %CPU COMMAND\n" + pid + " user 12.5 command\n", "12.50%"},
    {"PID USER %CPU COMMAND\n" + pid + " user\n", "0.00%"},
    {"PID USER %CPU COMMAND\n999999 user 99 command\n", "0.00%"},
    {"unexpected output\n", "0.00%"}
  };
  for (const auto& item : cases) {
    {
      std::ofstream file(script);
      file << "#!/bin/sh\n/bin/cat <<'DATA'\n" << item.first << "DATA\n";
    }
    EXPECT_EQ(chmod(script.c_str(), 0700), 0);
    const std::string path = dir + "/sample.csv";
    {
      GuestAgent agent(GuestAgentExternalInfo(path, nullptr, nullptr, 1, "top"));
      const std::string data = Sample(agent);
      EXPECT_NE(data.find(item.second), std::string::npos) << data;
    }
    std::remove(path.c_str());
  }
  if (had_path) setenv("PATH", saved_path.c_str(), 1); else unsetenv("PATH");
  std::remove(script.c_str());
  rmdir(dir.c_str());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
