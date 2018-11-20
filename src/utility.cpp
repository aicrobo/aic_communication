#include "utility.h"

namespace aicrobot
{

/**
 * @brief getTimestampNow    获取当前系统时间戳
 * @return                   时间戳(毫秒)
 */
int64_t getTimestampNow()
{
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

/**
 * @brief getDateTime        将时间戳转化为本地时间
 * @param milliseconds       时间戳(毫秒)
 * @return                   本地时间格式化字符串, 精确到毫秒
 */
std::string getDateTime(int64_t milliseconds)
{
  time_t t_stamp = milliseconds / 1000;

  struct tm t_struct;
  t_struct = *localtime(&t_stamp);

  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &t_struct);

  std::string date(buf);
  date = date + "." + std::to_string(milliseconds % 1000);

  return date;
}

/**
 * @brief getTid             获取当前线程标识号
 * @return                   线程标识号
 */
int getTid()
{
#ifdef __linux__
  return static_cast<int>(::syscall(SYS_gettid));
#else
  std::stringstream ss;
  ss << std::this_thread::get_id();
  return std::stoi(ss.str());
#endif
}

}