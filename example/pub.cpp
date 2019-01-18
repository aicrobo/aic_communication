#include "test.h"

int main()
{
  std::thread thr(aicrobot::thr_pub);
  thr.detach();
  getchar();
  return 0;
}