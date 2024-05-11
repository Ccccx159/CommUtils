#include "commutils/commutils.h"

int CpuUsageSimulate(const std::string & tag) {
  long long total_time = 100, run_time = 50;
  int run_cnt = 100;
  auto & tracer = Tracer::instance();
  while (run_cnt > 0) {
    tracer.begin(tag + std::to_string(getpid()));
    auto start = DateTime().GetMsTime();
    while (DateTime().GetMsTime() - start <= run_time) {};
    DateTime().SleepMs(total_time - run_time);
    run_cnt--;
    tracer.end(tag + std::to_string(getpid()));
  }
  return 0;
}


int main(int argc, char** argv) {
  std::thread t1(CpuUsageSimulate, "thread1_");
  std::thread t2(CpuUsageSimulate, "thread2_");

  t1.join();
  t2.join();
  return 0;
}