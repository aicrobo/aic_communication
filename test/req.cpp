#include "test.h"

int main()
{
  std::thread thr(aicrobot::thr_req);
  thr.detach();
  getchar();
  return 0;
}