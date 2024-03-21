#include "commutils/commutils.h"
#include "gtest/gtest.h"

bool validateCSVFileFormat(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cout << "Failed to open file: " << filename << std::endl;
    return false;
  }

  std::string line;
  std::getline(file, line);  // 读取第一行

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
  // 检查表头
  std::regex headerRegex("Time,ProcName,PID,CPU Usage,Mem Usage");
  if (!std::regex_match(line, headerRegex)) {
    return false;
  }

  // 检查数据行
  std::regex dataRegex(
      "\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2},\\w+,\\d+,\\d+\\.\\d{2}%,\\d+ "
      "KB");
  while (std::getline(file, line)) {
    if (!std::regex_match(line, dataRegex)) {
      return false;
    }
  }
#else
  // 检查表头
  std::string header("Time,ProcName,PID,CPU Usage,Mem Usage");
  if (line.find(header) == std::string::npos) {
    return false;
  }

  // 检查数据行
  while (std::getline(file, line)) {
    std::vector<std::string> fields;
    std::istringstream iss(line);
    std::string field;
    while (std::getline(iss, field, ',')) {
      fields.push_back(field);
    }

    if (fields.size() != 5) {
      return false;
    }

    // 检查日期和时间
    if (fields[0].size() != 19 || fields[0][4] != '-' || fields[0][7] != '-' ||
        fields[0][10] != ' ' || fields[0][13] != ':' || fields[0][16] != ':') {
      return false;
    }

    // 检查 PID
    if (!std::all_of(fields[2].begin(), fields[2].end(), ::isdigit)) {
      return false;
    }

    // 检查 CPU 使用率
    auto isFloatPercentage = [](const std::string& str) {
      if (str.back() != '%') {
        return false;
      }

      std::string numberPart = str.substr(0, str.size() - 1);
      std::istringstream iss(numberPart);
      float f;
      // noskipws considers leading whitespace invalid
      // Check the entire string was consumed and if either failbit or badbit is
      // set
      iss >> std::noskipws >> f;  
      return iss.eof() && !iss.fail();
    };
    if (!isFloatPercentage(fields[3])) {
      return false;
    }

    // 检查内存使用量
    if (fields[4].substr(fields[4].size() - 3) != " KB" ||
        !std::all_of(fields[4].begin(), fields[4].end() - 3, ::isdigit)) {
      return false;
    }
  }
#endif

  return true;
}

TEST(GuestAgentCSV, validateCSVFormat) {
  struct GuestAgentExternalInfo info("./validateCSVFormat.csv", nullptr,
                                     nullptr, 1);
  GuestAgent::GetGuestAgentInstance(info);
  std::this_thread::sleep_for(std::chrono::seconds(6));
  EXPECT_TRUE(validateCSVFileFormat("./validateCSVFormat.csv"));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}