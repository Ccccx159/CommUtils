#include "commutils/commutils.h"
#include "gtest/gtest.h"

bool validateFileFormat(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cout << "Failed to open file: " << filename << std::endl;
    return false;
  }

  std::string line;
  std::getline(file, line);  // 读取第一行

  // 检查表格的边框
  std::regex borderRegex("\\+[-]+\\+");
  if (!std::regex_match(line, borderRegex)) {
    return false;
  }

  // 检查表头
  std::getline(file, line);
  std::regex headerRegex(
      "\\|\\s*Time\\s*\\|\\s*ProcName\\s*\\|\\s*PID\\s*\\|\\s*CPU "
      "Usage\\s*\\|\\s*Mem Usage\\s*\\|");
  if (!std::regex_match(line, headerRegex)) {
    return false;
  }

  // 检查数据行
  std::regex dataRegex(
      "\\|\\s*\\d{4}-\\d{2}-\\d{2} "
      "\\d{2}:\\d{2}:\\d{2}\\s*\\|\\s*\\w+\\s*\\|\\s*\\d+\\s*\\|\\s*\\d+\\.\\d{"
      "2}%\\s*\\|\\s*\\d+ KB\\s*\\|");
  while (std::getline(file, line)) {
    if (std::regex_match(line, borderRegex)) {
      continue;
    }
    if (!std::regex_match(line, dataRegex)) {
      return false;
    }
  }

  return true;
}

TEST(GuestAgentTable, validateTableFormat) {
  struct GuestAgentExternalInfo info("./validateTableFormat.table", nullptr, nullptr, 1);
  GuestAgent::GetGuestAgentInstance(info);
  std::this_thread::sleep_for(std::chrono::seconds(6));
  EXPECT_TRUE(validateFileFormat("./validateTableFormat.table"));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}