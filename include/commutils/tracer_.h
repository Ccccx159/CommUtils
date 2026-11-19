#pragma once

#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

// Linux thread CPU tracing. stop() drains accepted events and closes the file.
class Tracer {
 private:
  struct Event {
    std::string name, cat, ph;
    int64_t ts = 0, tts = 0;
    uint64_t pid = 0, tid = 0;
    json args = json::object();
    int64_t involuntarily_preempted = 0, voluntarily_preempted = 0;
  };

 public:
  explicit Tracer(const std::string& path = "") {
    trace_file_path_ = path.empty()
        ? "./Tracer_Report_" + std::to_string(getpid()) + ".json" : path;
    of_trace_file_.open(trace_file_path_);
    if (!of_trace_file_) throw std::runtime_error("Trace report file open failed");
    begin_wall_offset_ = ClockNs(CLOCK_MONOTONIC);
    of_trace_file_ << "{\"traceEvents\":[\n";
    report_thread_ = std::thread(&Tracer::report, this);
  }

  ~Tracer() { stop(); }
  Tracer(const Tracer&) = delete;
  Tracer& operator=(const Tracer&) = delete;

  void begin(const std::string& tag, const json& args = json::object(),
             const std::string& cat = "generic", const std::string& phase = "B") {
    event_stamp(tag, args, cat, phase);
  }

  void end(const std::string& tag, const json& args = json::object(),
           const std::string& cat = "generic", const std::string& phase = "E") {
    event_stamp(tag, args, cat, phase);
  }

  // Call from an owning thread, after instrumented worker threads have finished.
  // Concurrent stop calls are allowed. Events submitted after stopping are ignored.
  void stop() {
    std::lock_guard<std::mutex> join_lock(stop_mutex_);
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      stop_ = true;
    }
    ready_.notify_one();
    if (report_thread_.joinable()) report_thread_.join();
  }

  static Tracer& instance() {
    static Tracer tracer;
    return tracer;
  }

  // Construct/destroy on the same thread, with the tracer outliving this scope.
  class ScopedEvent {
   public:
    explicit ScopedEvent(const std::string& tag,
                         const json& args = json::object(),
                         const std::string& cat = "generic")
        : ScopedEvent(Tracer::instance(), tag, args, cat) {}
    ScopedEvent(Tracer& tracer, const std::string& tag,
                const json& args = json::object(),
                const std::string& cat = "generic")
        : tracer_(tracer), tag_(tag), cat_(cat) {
      tracer_.begin(tag_, args, cat_);
    }
    ~ScopedEvent() {
      // Destruction must not interrupt exception unwinding in instrumented code.
      try { tracer_.end(tag_, json::object(), cat_); } catch (...) {}
    }
    ScopedEvent(const ScopedEvent&) = delete;
    ScopedEvent& operator=(const ScopedEvent&) = delete;
   private:
    Tracer& tracer_;
    std::string tag_, cat_;
  };

 private:
  static int64_t ClockNs(clockid_t clock) {
    timespec ts{};
    if (clock_gettime(clock, &ts) != 0)
      throw std::runtime_error("clock_gettime failed");
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
  }

  void event_stamp(const std::string& tag, const json& args,
                   const std::string& cat, const std::string& phase) {
    // The same lock gates shutdown and preserves each thread's submission order.
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (stop_) return;
    if (!args.is_object()) throw std::invalid_argument("Trace args must be an object");
    rusage usage{};
    if (getrusage(RUSAGE_THREAD, &usage) != 0)
      throw std::runtime_error("getrusage failed");
    Event e;
    e.name = tag;
    e.cat = cat;
    e.ph = phase;
    e.ts = ClockNs(CLOCK_MONOTONIC) - begin_wall_offset_;
    e.tts = ClockNs(CLOCK_THREAD_CPUTIME_ID);
    e.pid = static_cast<uint64_t>(getpid());
    e.tid = static_cast<uint64_t>(syscall(SYS_gettid));
    e.involuntarily_preempted = usage.ru_nivcsw;
    e.voluntarily_preempted = usage.ru_nvcsw;
    e.args = args;
    event_queue_.push_back(std::move(e));
    ready_.notify_one();
  }

  void report() {
    using Key = std::pair<uint64_t, std::string>;
    std::map<Key, std::vector<Event>> begins;
    bool first = true;
    for (;;) {
      Event e;
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        ready_.wait(lock, [this] { return stop_ || !event_queue_.empty(); });
        if (event_queue_.empty()) break;
        e = std::move(event_queue_.front());
        event_queue_.pop_front();
      }
      const Key key(e.tid, e.name);
      if (e.ph == "B") {
        begins[key].push_back(e);
      } else if (e.ph == "E") {
        auto it = begins.find(key);
        if (it != begins.end() && !it->second.empty()) {
          const Event& be = it->second.back();
          const int64_t wall_ns = e.ts - be.ts;
          const int64_t cpu_ns = e.tts - be.tts;
          e.args["wall_time_us"] = wall_ns / 1000.0;
          e.args["cpu_time_us"] = cpu_ns / 1000.0;
          e.args["dutycycle"] = wall_ns > 0 ? 100.0 * cpu_ns / wall_ns : 0.0;
          e.args["preempted"] = e.involuntarily_preempted - be.involuntarily_preempted;
          e.args["voluntary"] = e.voluntarily_preempted - be.voluntarily_preempted;
          it->second.pop_back();
          if (it->second.empty()) begins.erase(it);
        } else {
          e.args["unmatched_begin"] = true;
        }
      }
      // Chrome Trace Event timestamps are microseconds, including fractional parts.
      json record = {{"name", e.name}, {"cat", e.cat}, {"ph", e.ph},
                     {"ts", e.ts / 1000.0}, {"tts", e.tts / 1000.0},
                     {"pid", e.pid}, {"tid", e.tid}, {"args", e.args}};
      if (!first) of_trace_file_ << ",\n";
      of_trace_file_ << record;
      first = false;
    }
    of_trace_file_ << "\n]}\n";
    of_trace_file_.close();
  }

  std::deque<Event> event_queue_;
  std::mutex queue_mutex_, stop_mutex_;
  std::condition_variable ready_;
  bool stop_ = false;
  std::thread report_thread_;
  std::string trace_file_path_;
  std::ofstream of_trace_file_;
  int64_t begin_wall_offset_ = 0;
};
