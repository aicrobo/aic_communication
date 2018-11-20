#ifndef __AIC_COMMU_UTILITY_H__
#define __AIC_COMMU_UTILITY_H__
#ifdef __linux__
#include <unistd.h>
#include <sys/syscall.h>
#else
#include <sstream>
#pragma warning(disable : 4996)
#endif

#include <cstdio>
#include <chrono>
#include <memory>
#include <string>
#include <time.h>
#include <thread>

namespace aicrobot
{

#define SLEEP(millisec) std::this_thread::sleep_for(std::chrono::milliseconds(millisec));

/**
 * @brief stringFormat       字符串格式化函数, 提供类似 sprintf 的功能
 * @param format             格式化规则
 * @param args               格式化参数
 * @return
 */
template <typename... Args>
std::string stringFormat(const std::string &format, Args... args)
{
  size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  std::string msg(buf.get(), buf.get() + size - 1);
  return msg;
}

std::string getDateTime(int64_t milliseconds);
int64_t getTimestampNow();
int getTid();

} // namespace aicrobot

#endif // __AIC_COMMU_UTILITY_H__