#ifndef __AIC_COMMU_MONITOR_H__
#define __AIC_COMMU_MONITOR_H__

#include "zmq.hpp"
#include <map>

#define ZMQ_EVENT_MAP zmq::monitor_t_impl::map_event_to_str

namespace zmq
{
class monitor_t_impl : public monitor_t
{
public:
  monitor_t_impl() = default;
  ~monitor_t_impl() = default;

  /**
   * @brief Notify       通知回调
   * @param 参数一        事件结构
   * @param 参数二        地址信息
   * @return
   */
  using Notify = std::function<void(const zmq_event_t &, const std::string &)>;

  void setNotify(Notify func)
  {
    callback_ = func;
  }

  void doNotify(const zmq_event_t &event, const std::string &addr_msg)
  {
    if (callback_ != nullptr)
    {
      callback_(event, addr_msg);
    }
  }

public:
  // 下列 on_*** 样式的函数均继承自zmq::monitor_t

  virtual void on_monitor_started() {}

  virtual void on_event_connected(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_connect_delayed(const zmq_event_t &event_,
                                        const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_connect_retried(const zmq_event_t &event_,
                                        const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_listening(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_bind_failed(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_accepted(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_accept_failed(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_closed(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_close_failed(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_disconnected(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }
  virtual void on_event_unknown(const zmq_event_t &event_, const char *addr_)
  {
    doNotify(event_, addr_);
  }

public:
  static const std::map<uint16_t, const char *> map_event_to_str; // 事件与描述的映射

private:
  Notify callback_ = nullptr;
};

} // namespace zmq

#endif // __AIC_COMMU_MONITOR_H__