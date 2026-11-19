#pragma once

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "commutils/datetime_.h"
#include "commutils/file_.h"
#include "commutils/logger_.h"

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
      : agent_file_(agent_file), daemon_thread_fun_(func), arg_(arg),
        interval_(interval), cpu_method_(cpu_method),
        stop_state_(std::make_shared<StopState>()) {}

  // External callbacks receive this struct, with arg_ holding user data.
  // They must return when StopRequested() or WaitForStop() returns true.
  bool StopRequested() const { return stop_state_->requested.load(); }
  bool WaitForStop(int milliseconds) const {
    std::unique_lock<std::mutex> lock(stop_state_->mutex);
    return stop_state_->ready.wait_for(lock, std::chrono::milliseconds(milliseconds),
                                      [this] { return StopRequested(); });
  }

 private:
  struct StopState {
    std::atomic<bool> requested{false};
    std::mutex mutex;
    std::condition_variable ready;
  };
  std::shared_ptr<StopState> stop_state_;
  void RequestStop() {
    {
      std::lock_guard<std::mutex> lock(stop_state_->mutex);
      stop_state_->requested.store(true);
    }
    stop_state_->ready.notify_all();
  }
  friend class GuestAgent;
};

class GuestAgent {
 public:
  explicit GuestAgent(const GuestAgentExternalInfo& info = {})
      : external_info_(info) {
    if (info.interval_ <= 0) throw std::invalid_argument("Agent interval must be positive");
    external_info_.stop_state_ = std::make_shared<GuestAgentExternalInfo::StopState>();
    if (!external_info_.daemon_thread_fun_) {
      if (info.cpu_method_ != "stat" && info.cpu_method_ != "top")
        throw std::invalid_argument("CPU method must be stat or top");
      const std::string path = info.agent_file_.empty() ? "/dev/null" : info.agent_file_;
      ofs_agent_file_.open(path);
      if (!ofs_agent_file_) throw std::runtime_error("Agent file open failed");
      csv_ = FileOperate().GetSuffix(path) == "csv";
      pid_ = getpid();
      std::ifstream comm("/proc/self/comm");
      std::getline(comm, proc_name_);
    }
    daemon_ = std::thread([this] {
      try {
        if (external_info_.daemon_thread_fun_)
          external_info_.daemon_thread_fun_(static_cast<void*>(&external_info_));
        else
          Run();
      } catch (const std::exception& error) {
        LOG(ERROR) << "GuestAgent worker failed:" << error.what();
      } catch (...) {
        LOG(ERROR) << "GuestAgent worker failed with an unknown exception";
      }
      external_info_.RequestStop();
    });
  }

  ~GuestAgent() { Stop(); }
  GuestAgent(const GuestAgent&) = delete;
  GuestAgent& operator=(const GuestAgent&) = delete;

  // Call from an owning thread. External callbacks must cooperate with shutdown.
  void Stop() {
    std::lock_guard<std::mutex> stop_lock(stop_mutex_);
    external_info_.RequestStop();
    if (daemon_.joinable()) daemon_.join();
    std::lock_guard<std::mutex> file_lock(file_mutex_);
    if (ofs_agent_file_.is_open()) ofs_agent_file_.close();
  }

  static GuestAgent& GetGuestAgentInstance(const GuestAgentExternalInfo& info = {}) {
    static GuestAgent instance(info);
    return instance;
  }

  std::string ReadAgentFile() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (ofs_agent_file_.is_open()) ofs_agent_file_.flush();
    const std::string path = external_info_.agent_file_.empty()
        ? "/dev/null" : external_info_.agent_file_;
    std::ifstream file(path);
    if (!file) return "";
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
  }

 private:
  static std::string Percent(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value << '%';
    return out.str();
  }

  std::string GetVmRss() {
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
      if (line.compare(0, 6, "VmRSS:") != 0) continue;
      std::istringstream fields(line.substr(6));
      unsigned long long kb = 0;
      if (fields >> kb) return std::to_string(kb);
    }
    return "0";
  }

  std::string GetCpuUsageByStat() {
    std::ifstream stat("/proc/self/stat");
    std::string line;
    if (!std::getline(stat, line)) return Percent(0);
    // comm (field 2) may contain spaces and parentheses; split after its last ')'.
    const size_t close = line.rfind(')');
    if (close == std::string::npos) return Percent(0);
    std::istringstream fields(line.substr(close + 1));
    std::vector<std::string> tokens{std::istream_iterator<std::string>(fields),
                                    std::istream_iterator<std::string>()};
    if (tokens.size() <= 19) return Percent(0);
    try {
      const double ticks = std::stod(tokens[11]) + std::stod(tokens[12]);
      const double start = std::stod(tokens[19]);
      const long hertz = sysconf(_SC_CLK_TCK);
      double uptime = 0;
      std::ifstream uptime_file("/proc/uptime");
      if (hertz <= 0 || !(uptime_file >> uptime)) return Percent(0);
      const double elapsed = uptime - start / hertz;
      return Percent(elapsed > 0 ? 100.0 * ticks / hertz / elapsed : 0);
    } catch (const std::exception&) { return Percent(0); }
  }

  std::string GetCpuUsageByTop() {
    // One batch-mode snapshot, fixed locale, exact PID match and checked columns.
    const std::string cmd = "LC_ALL=C top -b -n 1 -p " + std::to_string(pid_);
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return Percent(0);
    std::string output;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe)) output += buffer;
    const int result = pclose(pipe);
    if (result != 0) return Percent(0);
    std::istringstream lines(output);
    std::string line;
    size_t cpu_slot = 0;
    bool header = false;
    while (std::getline(lines, line)) {
      std::istringstream fields(line);
      const std::vector<std::string> tokens{std::istream_iterator<std::string>(fields),
                                            std::istream_iterator<std::string>()};
      if (tokens.empty()) continue;
      if (tokens[0] == "PID") {
        header = false;
        for (size_t i = 0; i < tokens.size(); ++i) {
          if (tokens[i] == "%CPU") { cpu_slot = i; header = true; break; }
        }
      } else if (header && tokens[0] == std::to_string(pid_) && cpu_slot < tokens.size()) {
        try { return Percent(std::stod(tokens[cpu_slot])); }
        catch (const std::exception&) { return Percent(0); }
      }
    }
    return Percent(0);
  }

  static std::string CsvField(const std::string& field) {
    if (field.find_first_of(",\"\r\n") == std::string::npos) return field;
    std::string escaped = "\"";
    for (char ch : field) { if (ch == '"') escaped += '"'; escaped += ch; }
    return escaped + '"';
  }

  std::string Row(const std::string& time, const std::string& name,
                  const std::string& pid, const std::string& cpu,
                  const std::string& mem) {
    if (csv_) return CsvField(time) + ',' + CsvField(name) + ',' + CsvField(pid)
        + ',' + CsvField(cpu) + ',' + CsvField(mem);
    return fmt::format("| {:^19} | {:^10} | {:^10} | {:^10} | {:^15} |",
                       time, name.substr(0, 10), pid, cpu, mem);
  }

  void Run() {
    const std::string header = Row("Time", "ProcName", "PID", "CPU Usage", "Mem Usage");
    const std::string border = "+" + std::string(header.size() - 2, '-') + "+";
    {
      std::lock_guard<std::mutex> lock(file_mutex_);
      if (!csv_) ofs_agent_file_ << border << '\n';
      ofs_agent_file_ << header << '\n';
      if (!csv_) ofs_agent_file_ << border << '\n';
    }
    while (!external_info_.StopRequested()) {
      // Sampling may invoke top; keep file reads independent of that work.
      const std::string cpu = external_info_.cpu_method_ == "stat"
          ? GetCpuUsageByStat() : GetCpuUsageByTop();
      const std::string row = Row(DateTime().GetFmtTime(), proc_name_,
                                  std::to_string(pid_), cpu, GetVmRss() + " KB");
      {
        std::lock_guard<std::mutex> lock(file_mutex_);
        ofs_agent_file_ << row << '\n';
        if (!csv_) ofs_agent_file_ << border << '\n';
        ofs_agent_file_.flush();
      }
      // Interruptible waits keep destruction fast even with long sample intervals.
      std::unique_lock<std::mutex> lock(external_info_.stop_state_->mutex);
      external_info_.stop_state_->ready.wait_for(
          lock, std::chrono::seconds(external_info_.interval_),
          [this] { return external_info_.StopRequested(); });
    }
  }

  GuestAgentExternalInfo external_info_;
  std::thread daemon_;
  pid_t pid_ = 0;
  std::string proc_name_;
  std::ofstream ofs_agent_file_;
  bool csv_ = false;
  std::mutex file_mutex_, stop_mutex_;
};
