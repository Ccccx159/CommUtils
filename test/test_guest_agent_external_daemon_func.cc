#include "commutils/commutils.h"
#include "gtest/gtest.h"

void* ExternalDaemonFunc(void* arg) {
  struct GuestAgentExternalInfo* info =
      static_cast<struct GuestAgentExternalInfo*>(arg);

  std::ofstream ofs_agent_file(info->agent_file_);
  if (!ofs_agent_file.is_open()) {
    LOG(ERROR) << "Agent file " << info->agent_file_ << " open failed";
    throw std::runtime_error("Agent file open failed");
  }
  while (true) {
    ofs_agent_file << "Hello, world!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return nullptr;
}

int InitGuestAgent() {
  struct GuestAgentExternalInfo info(
      "./validateExternalFormat.log", ExternalDaemonFunc,
      nullptr);
  GuestAgent::GetGuestAgentInstance(info);
  return 0;
}

TEST(GuestAgentExternalDaemonFunc, validateExternalFormat) {
  InitGuestAgent();
  std::this_thread::sleep_for(std::chrono::seconds(6));
  std::string agent_file = "./validateExternalFormat.log";
  std::ifstream ifs_agent_file(agent_file);
  for (int i = 0; i < 6 / 1; i++) {
    std::string line;
    std::getline(ifs_agent_file, line);
    EXPECT_EQ(line, "Hello, world!");
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}