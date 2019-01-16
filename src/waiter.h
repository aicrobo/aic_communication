#ifndef __AIC_COMMU_WAITER_H__
#define __AIC_COMMU_WAITER_H__

#include <mutex>
#include <condition_variable>
#include <stdlib.h>
#include <memory>


namespace aicrobot
{

class Waiter
{
public:
  Waiter() = default;
  ~Waiter() noexcept = default;
  Waiter(const Waiter &) = delete;
  Waiter &operator=(const Waiter &) = delete;
  Waiter(Waiter &&) = delete;
  Waiter &operator=(Waiter &&) = delete;

  bool signaled(){return signaled_;}
  void set_signaled(bool signaled){signaled_ = signaled;}

  /**
   * @brief wait            等待信号唤醒
   * @return
   */
  void wait()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    signaled_ = false;
    cond_.wait(lock, [&] { return signaled_; });
  }

  /**
   * @brief broadcast       发送唤醒信号
   * @return
   */
  void broadcast()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    signaled_ = true;
    cond_.notify_all();
  }

private:
  std::mutex mutex_;
  std::condition_variable cond_;
  bool signaled_ = false;
};

using WaiterPtr = std::shared_ptr<Waiter>;

} // namespace aicrobot

#endif // __AIC_COMMU_WAITER_H__
