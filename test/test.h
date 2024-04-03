#pragma once
#include "commutils/commutils.h"

inline void setLogger() {
  std::string pattern = "[%Y-%m-%d %T] [%^%l%$] [%s:%#] [%!] %v";

  INITIALIZE_DEFAULT_LOGGER(pattern, ERROR);
}