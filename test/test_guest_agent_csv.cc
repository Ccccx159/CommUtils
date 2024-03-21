#include "commutils/commutils.h"
#include "gtest/gtest.h"

bool validateCSVFileFormat(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::getline(file, line); // 读取第一行

    // 检查表头
    std::regex headerRegex("Time,ProcName,PID,CPU Usage,Mem Usage");
    if (!std::regex_match(line, headerRegex)) {
        return false;
    }

    // 检查数据行
    std::regex dataRegex("\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2},\\w+,\\d+,\\d+\\.\\d{2}%,\\d+ KB");
    while (std::getline(file, line)) {
        if (!std::regex_match(line, dataRegex)) {
            return false;
        }
    }

    return true;
}

TEST(GuestAgentCSV, validateCSVFormat) {
    struct GuestAgentExternalInfo info("./validateCSVFormat.csv", nullptr, nullptr, 1);
    GuestAgent::GetGuestAgentInstance(info);
    std::this_thread::sleep_for(std::chrono::seconds(6));
    EXPECT_TRUE(validateCSVFileFormat("./test_guest_agent_csv.csv"));
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}