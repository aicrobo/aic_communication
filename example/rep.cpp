#include "test.h"

int main()
{
  std::thread thr(aicrobot::thr_rep);
  thr.detach();
  getchar();
  return 0;
}