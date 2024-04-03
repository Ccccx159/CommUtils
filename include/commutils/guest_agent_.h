#pragma once

#include <chrono>
#include <mutex>
#include <thread>

#include "commutils.h"

// #ifndef GUEST_AGENT_TO_FILE
// #define GUEST_AGENT_TO_FILE "/dev/null"
// #endif

struct GuestAgentExternalInfo {
  std::string agent_file_;
  std::function<void*(void*)> daemon_thread_fun_;
  void* arg_;
  int interval_;
  std::string cpu_method_;

  GuestAgentExternalInfo(std::string agent_file = "./guest_agent.csv",
                         std::function<void*(void*)> func = nullptr,
                         void* arg = nullptr, int interval = 10,
                         std::string cpu_method = "top")
      : agent_file_(agent_file),
        daemon_thread_fun_(func),
        arg_(arg),
        interval_(interval),
        cpu_method_(cpu_method) {}
};

class GuestAgent {
 private:
  enum class Format {
    UNKNOWN = 0,
    TXT = UNKNOWN,
    LOG = UNKNOWN,
    TABLE = UNKNOWN,
    CSV,
  };

 private:
  GuestAgent(struct GuestAgentExternalInfo const& info) : external_info_(info) {
    LOG(INFO) << "GuestAgent Constructor";

    if (external_info_.daemon_thread_fun_ != nullptr) {
      daemon_ = std::thread(external_info_.daemon_thread_fun_,
                            static_cast<void*>(&external_info_));
      daemon_.detach();
    } else {
      std::string agent_file = external_info_.agent_file_.size() == 0
                                   ? "/dev/null"
                                   : external_info_.agent_file_;
      ofs_agent_file_.open(agent_file);
      if (!ofs_agent_file_.is_open()) {
        LOG(ERROR) << "Agent file " << agent_file << " open failed";
        throw std::runtime_error("Agent file open failed");
      }
      if (FileOperate().GetSuffix(agent_file) == "csv") {
        fmt_type_ = Format::CSV;
      } else {
        fmt_type_ = Format::TABLE;
      }

      pid_ = getpid();
      proc_name_ = GetProcName();
      daemon_ = std::thread(&GuestAgent::Run, this);
      daemon_.detach();
    }
  }

  ~GuestAgent() {
    if (external_info_.daemon_thread_fun_ == nullptr) {
      ofs_agent_file_.close();
    }
    LOG(INFO) << "GuestAgent Destructor";
  }

  std::string GetProcName() {
    std::ifstream commFile("/proc/self/comm");
    if (!commFile.is_open()) {
      LOG(ERROR) << "Failed to open /proc/self/comm";
      return "";
    }
    commFile >> proc_name_;
    commFile.close();
    return proc_name_;
  }

  std::string GetVmRss() {
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
      LOG(ERROR) << "Failed to open /proc/self/status";
      return "";
    }
    std::string line;
    while (std::getline(statusFile, line)) {
      if (line.find("VmRSS") != std::string::npos) {
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
        // 使用正则表达式提取数字
        std::regex e("\\d+");
        std::smatch m;
        std::regex_search(line, m, e);
        line = m.str();
#else
        // VmRSS:    15504 kB
        line = line.substr(0, line.rfind(" "));
        line = line.substr(line.rfind(" ") + 1);
#endif
        break;
      }
    }
    statusFile.close();
    return line;
  }

  // CPU 使用率提供两种获取方法：
  // 1. 通过 /proc/<pid>/stat 中的 cpu 执行时间进行计算整个进程的平均 CPU 使用率
  // 2. 通过 `top -n 1` 获取 "%CPU" 字段值
  // 默认使用 top 方法，可通过初始化 Guest_Agent 实例时对
  // external_info::cpu_method_ 赋值 "stat" 或 "top" 选择使用哪种方法获取 CPU
  // 使用率。

  // 获取当前进程Cpu使用率
  std::string GetCpuUsageByStat() {
    std::ifstream statFile("/proc/self/stat");
    if (!statFile.is_open()) {
      LOG(ERROR) << "Failed to open /proc/self/stat";
      return "";
    }
    std::string line;
    std::getline(statFile, line);
    statFile.close();
    std::istringstream iss(line);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};
    // 获取用户态和内核态的时间
    long utime = std::stol(tokens[13]);
    long stime = std::stol(tokens[14]);
    long cutime = std::stol(tokens[15]);
    long cstime = std::stol(tokens[16]);
    long starttime = std::stol(tokens[21]);
    long Hertz = sysconf(_SC_CLK_TCK);
    long total_time = utime + stime + cutime + cstime;
    long seconds = DateTime().GetSTime() - starttime / Hertz;
    float cpu_usage = 100 * ((total_time * 1.0 / Hertz) / seconds);
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << cpu_usage;
    return stream.str() + "%";
  }

// 通过 top -n 1 | grep "PID" 获取进程的CPU使用率
#include <cstdlib>  // Include the necessary header file

  std::string ExecCmd(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
      char buffer[128];
      while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) {
          result += buffer;
        }
      }
      pclose(pipe);
    }
    return result;
  }

  std::string GetCpuUsageByTop() {
    // top -n 1 | grep -i "%cpu" 获取 cpu使用率所在槽位
    std::string cmd = "top -n 1 | grep --color=never \"%CPU\"";
    std::string line = ExecCmd(cmd);
    if (line.empty()) {
      return "0.00%";
    }
    std::istringstream iss(line);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};
    // 移除包含非打印字符的元素
    tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                                [](const std::string& token) {
                                  return std::any_of(
                                      token.begin(), token.end(),
                                      [](char c) { return ::iscntrl(c); });
                                }),
                 tokens.end());
    long unsigned int slot = 0;
    for (long unsigned int i = 0; i < tokens.size(); i++) {
      if (tokens[i] == "%CPU") {
        slot = i;
        break;
      }
    }

    // 获取当前进程的CPU使用率
    cmd = "top -n 1 | grep --color=never " + std::to_string(pid_);
    line = ExecCmd(cmd);
    if (line.empty()) {
      return "0.00%";
    }
    iss = std::istringstream(line);
    tokens = std::vector<std::string>{std::istream_iterator<std::string>{iss},
                                      std::istream_iterator<std::string>{}};
    // 移除包含非打印字符的元素
    tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                                [](const std::string& token) {
                                  return std::any_of(
                                      token.begin(), token.end(),
                                      [](char c) { return ::iscntrl(c); });
                                }),
                 tokens.end());
    return tokens[slot] + "%";
  }

  std::string generateTableRow(const std::string& time,
                               const std::string& procName,
                               const std::string& pid,
                               const std::string& cpuUsage,
                               const std::string& memUsage) {
    std::string row;
    std::string fmt_str;
    if (fmt_type_ == Format::CSV) {
      fmt_str = "{},{},{},{},{}";
    } else {
      fmt_str = "| {:^19} | {:^10} | {:^10} | {:^10} | {:^15} |";
    }

    row = fmt::format(fmt_str, time,
                      procName.size() > 10 ? procName.substr(0, 10) : procName,
                      pid, cpuUsage, memUsage);
    return row;
  }

  void Run() {
    std::string header =
        generateTableRow("Time", "ProcName", "PID", "CPU Usage", "Mem Usage");
    if (fmt_type_ == Format::CSV) {
      ofs_agent_file_ << header << std::endl;
    } else {
      ofs_agent_file_ << "+" << std::string(header.size() - 2, '-') << "+"
                      << std::endl;
      ofs_agent_file_ << header << std::endl;
      ofs_agent_file_ << "+" << std::string(header.size() - 2, '-') << "+"
                      << std::endl;
    }
    while (true) {
      std::unique_lock<std::mutex> uni_lock(file_lock_);
      if (external_info_.cpu_method_ == "stat") {
        ofs_agent_file_ << generateTableRow(DateTime().GetFmtTime(), proc_name_,
                                            std::to_string(pid_),
                                            GetCpuUsageByStat(),
                                            GetVmRss() + " KB")
                        << std::endl;
      } else {
        ofs_agent_file_ << generateTableRow(DateTime().GetFmtTime(), proc_name_,
                                            std::to_string(pid_),
                                            GetCpuUsageByTop(),
                                            GetVmRss() + " KB")
                        << std::endl;
      }

      if (fmt_type_ != Format::CSV) {
        ofs_agent_file_ << "+" << std::string(header.size() - 2, '-') << "+"
                        << std::endl;
      }
      uni_lock.unlock();
      std::this_thread::sleep_for(
          std::chrono::seconds(external_info_.interval_));
    }
  }

 private:
  std::thread daemon_;
  pid_t pid_;
  std::string proc_name_;
  std::ofstream ofs_agent_file_;
  Format fmt_type_;
  struct GuestAgentExternalInfo external_info_;
  std::mutex file_lock_;

 public:
  static GuestAgent& GetGuestAgentInstance(
      struct GuestAgentExternalInfo const& ex_info = {}) {
    static GuestAgent ins(ex_info);
    return ins;
  }

  std::string ReadAgentFile() {
    std::unique_lock<std::mutex> uni_lock(file_lock_);
    std::ifstream ifs_agent_file(external_info_.agent_file_);
    if (!ifs_agent_file.is_open()) {
      LOG(ERROR) << "open agent file " << external_info_.agent_file_
                 << " failed!";
      return "";
    }
    std::string ctx, line;
    while (std::getline(ifs_agent_file, line)) {
      ctx += line;
      ctx += "\n";
    }
    return ctx;
  }
};
