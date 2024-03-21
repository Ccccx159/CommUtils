#include "commutils/commutils.h"
#include "gtest/gtest.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)

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

#else

bool validateFileFormat(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cout << "Failed to open file: " << filename << std::endl;
    return false;
  }

  std::string line;
  std::getline(file, line);  // 读取第一行

  // 检查表格的边框
  if (line.find("+---") == std::string::npos) {
    return false;
  }

  // 检查表头
  std::getline(file, line);
  std::vector<std::string> headers = {"Time", "ProcName", "PID", "CPU Usage", "Mem Usage"};
  for (const auto& header : headers) {
    if (line.find(header) == std::string::npos) {
      return false;
    }
  }

  // 检查数据行
  auto isFloatPercentage = [](const std::string& str) {
    if (str.back() != '%') {
        return false;
    }

    std::string numberPart = str.substr(0, str.size() - 1);
    std::istringstream iss(numberPart);
    float f;
    iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
    // Check the entire string was consumed and if either failbit or badbit is set
    return iss.eof() && !iss.fail();
  };

  while (std::getline(file, line)) {
    if (line.find("+---") != std::string::npos) {
      continue;
    }

    std::vector<std::string> fields;
    std::istringstream iss(line);
    std::string field;
    while (std::getline(iss, field, '|')) {
        fields.push_back(field);
    }

    if (fields.size() != 6) {
        return false;
    }

    // 检查日期和时间
    std::string time = fields[1];
    if (time.size() != 21 || time[5] != '-' || time[8] != '-' || time[11] != ' ' || time[14] != ':' || time[17] != ':') {
        return false;
    }

    // 检查 PID
    std::string pid = fields[3];
    pid.erase(remove_if(pid.begin(), pid.end(), ::isspace), pid.end());
    if (!std::all_of(pid.begin(), pid.end(), ::isdigit)) {
        return false;
    }

    // 检查 CPU 使用率
    std::string cpuUsage = fields[4];
    cpuUsage.erase(remove_if(cpuUsage.begin(), cpuUsage.end(), ::isspace), cpuUsage.end());
    if (!isFloatPercentage(cpuUsage)) {
        return false;
    }

    // 检查内存使用量
    std::string memUsage = fields[5];
    memUsage.erase(remove_if(memUsage.begin(), memUsage.end(), ::isspace), memUsage.end());
    if (memUsage.substr(memUsage.size() - 2) != "KB" || !std::all_of(memUsage.begin(), memUsage.end() - 2, ::isdigit)) {
        return false;
    }
  }

  return true;
}

#endif

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