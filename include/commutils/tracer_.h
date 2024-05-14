#pragma once

#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>

#include <string>

#include "commutils.h"
using json = nlohmann::json;

class Tracer {
 private:
  struct Event {
    std::string name;
    std::string cat;
    std::string ph;
    uint64_t ts;
    uint64_t tts;
    uint64_t pid;
    uint64_t tid;
    json args;

    Event() {
      ts = 0;
      tts = 0;
      pid = 0;
      tid = 0;
      args = json::object();
    }
  };

  using ccque = moodycamel::ConcurrentQueue<Tracer::Event>;

 public:
  void begin(const std::string& tag, const std::string& cat = "generic",
             const std::string& phase = "B") {
    event_stamp(tag, cat, phase);
  }

  void end(const std::string& tag, const std::string& cat = "generic",
           const std::string& phase = "E") {
    event_stamp(tag, cat, phase);
  }

  void stop() {
    // TODO:
    stop_ = true;
    return;
  }

  static Tracer& instance() {
    static Tracer tracer;
    return tracer;
  }

 private:
  Tracer() : stop_(true) {
    trace_file_path_ = "./Tracer_Report_" + std::to_string(getpid()) + ".json";
    struct timespec wt, ct;
    clock_gettime(CLOCK_MONOTONIC, &wt);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ct);
    begin_wall_offset_ = wt.tv_sec * 1000000000L + wt.tv_nsec;
    struct timespec res;
    clock_getres(CLOCK_THREAD_CPUTIME_ID, &res);
    fprintf(stderr, "ThreadTracer: clock resolution: %ld nsec.\n", res.tv_nsec);
    begin_wall_cutoff_ = begin_wall_offset_;
    stop_ = false;

    // TODO:
    report_thread_ = std::thread(&Tracer::report, this);
  }

  virtual ~Tracer() {
    // TODO
    if (!stop_) {
      stop_ = true;
    }
    report_thread_.join();
    if (of_trace_file_.is_open()) {
      of_trace_file_.close();
    }
  }

  void report() {
    // TODO
    // 写入事件头信息
    of_trace_file_.open(trace_file_path_);
    if (!of_trace_file_.is_open()) {
      stop_ = true;
      LOG(ERROR) << "Trace report file open failed! " << trace_file_path_;
      return;
    }
    of_trace_file_ << "{\"traceEvents\":[\n";
    while (!stop_) {
      struct Event e;
      bool res = event_queue_.try_dequeue(e);
      if (!res) {
        DateTime().SleepMs(5);
        continue;
      }
      json je = json{{"name", e.name}, {"cat", e.cat},  {"ph", e.ph},
                     {"ts", e.ts},     {"tts", e.tts},  {"pid", e.pid},
                     {"tid", e.tid},   {"args", e.args}};
      if (0 != event_cnt_) {
        of_trace_file_ << ",\n";
      }
      of_trace_file_ << je;
      event_cnt_++;

      DateTime().SleepMs(5);
    }
    // 写入事件尾信息
    of_trace_file_ << "\n]}\n";
    of_trace_file_.close();
    return;
  }

  int event_stamp(const std::string& tag, const std::string& cat,
                  const std::string& phase) {
    if (stop_) {
      return 0;
    }
    const pid_t tid = static_cast<pid_t>(::syscall(SYS_gettid));
    struct timespec wt, ct;
    clock_gettime(CLOCK_MONOTONIC, &wt);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ct);
    struct rusage ru;
    int rv;
    rv = getrusage(RUSAGE_THREAD, &ru);
    if (rv < 0) {
      stop_ = true;
      fprintf(stderr, "ThreadTracer: rusage() failed. Stopped Recording.\n");
      return -1;
    }
    const int64_t wall_nsec = wt.tv_sec * 1000000000 + wt.tv_nsec;
    const int64_t cpu_nsec = ct.tv_sec * 1000000000 + ct.tv_nsec;

    if (wall_nsec < begin_wall_cutoff_) {
      return -1;
    }

    struct Event e;
    e.name = tag;
    e.cat = cat;
    e.ph = phase;
    e.ts = wall_nsec - begin_wall_offset_;
    e.tts = cpu_nsec;
    e.pid = static_cast<uint64_t>(getpid());
    e.tid = static_cast<uint64_t>(tid);

    event_queue_.enqueue(e);
    return 0;
  }

 private:
  ccque event_queue_;
  std::thread report_thread_;
  std::atomic<bool> stop_;
  std::string trace_file_path_;
  std::ofstream of_trace_file_;
  int64_t event_cnt_ = 0;
  int64_t begin_wall_offset_ = 0;
  int64_t begin_wall_cutoff_ = 0;
};
