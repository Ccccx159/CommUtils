#include "commutils/commutils.h"
#include "gtest/gtest.h"

int main() {
  
  INITIALIZE_DEFAULT_LOGGER(LOG_DEFAULT_PATTERN, TRACE);
  LOG(ERROR) << "This is an err message";
  LOG(TRACE) << "This is a trace message";
  LOG(DEBUG) << "This is a debug message";
  LOG(INFO) << "This is an info message";
  LOG(WARN) << "This is a warn message";
  LOG(ERROR) << "This is an err message";
  LOG(FATAL) << "This is a fatal message";

  return 0;
}