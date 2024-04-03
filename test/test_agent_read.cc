#include "commutils/commutils.h"

int main() {
  struct GuestAgentExternalInfo info("./validateCSVFormat.csv", nullptr,
                                     nullptr, 1);
  auto & ga = GuestAgent::GetGuestAgentInstance(info);
  DateTime().SleepMs(6*1000);
  std::string agent_ctx = ga.ReadAgentFile();
  LOG(WARN) << agent_ctx;
  return 0;
}