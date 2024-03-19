#include "commutils/commutils.h"
#include "test.h"

int main(int argc, char** argv) {
  int *p = nullptr;
  *p = 1;
  LOG(INFO) << fmt::format("pointer var p is: {:p}, and its value is {}", fmt::ptr(p), *p) ;
  return 0;
}