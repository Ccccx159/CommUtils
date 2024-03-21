#pragma once

#include <chrono>
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

  GuestAgentExternalInfo(std::string agent_file = "./guest_agent.csv",
                         std::function<void*(void*)> func = nullptr,
                         void* arg = nullptr, int interval = 10)
      : agent_file_(agent_file),
        daemon_thread_fun_(func),
        arg_(arg),
        interval_(interval) {}
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

  // // 2024-03-20 13:48:51
  // std::string GetCurFmtTime() {
  //   auto now = std::chrono::system_clock::now();
  //   auto now_c = std::chrono::system_clock::to_time_t(now);
  //   std::tm* now_tm = std::localtime(&now_c);
  //   std::stringstream ss;
  //   ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");
  //   return ss.str();
  // }

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

  // 获取当前进程Cpu使用率
  std::string GetCpuUsage() {
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
    long seconds = time(nullptr) - starttime / Hertz;
    float cpu_usage = 100 * ((total_time / Hertz) / seconds);
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << cpu_usage;
    return stream.str() + "%";
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
      ofs_agent_file_ << generateTableRow(DateTime().GetFmtTime(), proc_name_,
                                          std::to_string(pid_), GetCpuUsage(),
                                          GetVmRss() + " KB")
                      << std::endl;
      if (fmt_type_ != Format::CSV) {
        ofs_agent_file_ << "+" << std::string(header.size() - 2, '-') << "+"
                        << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::seconds(external_info_.interval_));
    }
  }

 private:
  std::thread daemon_;
  pid_t pid_;
  std::string proc_name_;
  std::ofstream ofs_agent_file_;
  Format fmt_type_;
  struct GuestAgentExternalInfo external_info_;

 public:
  static GuestAgent& GetGuestAgentInstance(
      struct GuestAgentExternalInfo const& ex_info = {}) {
    static GuestAgent ins(ex_info);
    return ins;
  }
};
