#include "test.h"

int main()
{
  std::thread thr(aicrobot::thr_sub);
  thr.detach();
  getchar();
  return 0;
}