#ifndef __AIC_COMMU_MONITOR_H__
#define __AIC_COMMU_MONITOR_H__

#include "zmq.hpp"
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include "waiter.h"
#include "utility.h"
#include <iostream>
#include "aic_commu.h"

#define ZMQ_EVENT_MAP zmq::monitor_t_impl::map_event_to_str
using namespace aicrobot;

namespace zmq
{

class monitor_t_impl : public monitor_t
{
public:
    struct NotifyData{
        zmq_event_t event;
        std::string addr_msg;
    };

    monitor_t_impl(){
        notify_mutex_ptr_ = std::make_shared<std::mutex>();
        notify_queue_ptr_ = std::make_shared<std::queue<NotifyData>>();
        exit_ptr_         = std::make_shared<bool>(false);
        status_call_ptr_  = std::make_shared<aicrobot::StatusCall>(nullptr);

        std::shared_ptr<std::mutex> notify_mutex_ptr    = notify_mutex_ptr_;
        std::shared_ptr<std::queue<NotifyData>> notify_queue_ptr = notify_queue_ptr_;
        std::shared_ptr<bool> exit_ptr          = exit_ptr_;
        std::shared_ptr<aicrobot::StatusCall>    status_call     = status_call_ptr_;

        //启动发送通知线程
        std::thread notify_thread( [notify_mutex_ptr,notify_queue_ptr,exit_ptr,status_call](){
            while(1){
                if(*exit_ptr)
                    break;
                SLEEP(100);
                NotifyData notify;
                {
                    std::lock_guard<std::mutex> lock(*notify_mutex_ptr);
                    if(notify_queue_ptr->empty())
                        continue;
                    notify = notify_queue_ptr->front();
                    notify_queue_ptr->pop();
                }

                if (status_call != nullptr && *status_call != nullptr)
                {
                  monitor_t_impl::socketStatusNotify(notify.event, notify.addr_msg,*status_call);

                }
            }

//            std::cout<<"socket status notify thread exit"<<std::endl;

        });
        notify_thread.detach();
    }
    ~monitor_t_impl(){*exit_ptr_ = true;}

    /**
     * @brief callLog       格式化日志, 并转发回调函数
     * @param level              日志级别
     * @param format             格式化规则
     * @param args               格式化参数
     * @return
     */
    template <typename... Args>
    void callLog(AicCommuLogLevels level, const std::string &format, Args... args)
    {
      if (static_cast<int>(level) < static_cast<int>(log_level_))
        return;
      std::string msg = stringFormat(format, args...);
      if (is_log_time_)
        msg = stringFormat("[%s]", getDateTime(getTimestampNow()).c_str()) + msg;

      if (log_call_ != nullptr)
        log_call_(msg);
      else
        printf("%s", msg.c_str());
    }

  /**
   * @brief Notify       通知回调
   * @param 参数一        事件结构
   * @param 参数二        地址信息
   * @return
   */
//  using Notify = std::function<void(const zmq_event_t &, const std::string &)>;

    /**
   * @brief setStatusCall   设置socket状态通知回调
   * @param func
   */
  void setStatusCall(aicrobot::StatusCall func){
    if(status_call_ptr_ != nullptr)
        *status_call_ptr_ = func;
  }

  /**
   * @brief setLogCall  设置日志回调
   * @param func    回调函数地址
   * @param level   日志级别
   * @param is_log_time 是否打印日志时间
   */
  void setLogCall(LogCall func, AicCommuLogLevels level, bool is_log_time)
  {
    log_call_ = func;
    log_level_ = level;
    is_log_time_ = is_log_time;
  }

  /**
   * @brief doNotify    把socket状态事件添加到通知队列里
   * @param event       socket事件
   * @param addr_msg    对应的地址
   */
  void doNotify(const zmq_event_t &event, const std::string &addr_msg)
  {

    if(map_event_to_str.find(event.event) != map_event_to_str.end()){
        callLog(AicCommuLogLevels::DEBUG, "[tid:%d] event:%s addr:%s\n",
                  getTid(), map_event_to_str.at(event.event),addr_msg.c_str());
    }else{
        callLog(AicCommuLogLevels::DEBUG, "[tid:%d] event:unkown_event addr:%s\n",
                  getTid(),addr_msg.c_str());
    }

    if (status_call_ptr_ != nullptr && *status_call_ptr_ != nullptr)
    {
        std::lock_guard<std::mutex> lock(*notify_mutex_ptr_);
        notify_queue_ptr_->push(NotifyData{event,addr_msg});
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
  static void invokeStatusCall(aicrobot::AicCommuStatus status, const std::string &addr, aicrobot::StatusCall func);
  static void socketStatusNotify(const zmq_event_t &et, const std::string &addr, aicrobot::StatusCall func);



private:

  std::shared_ptr<aicrobot::StatusCall>       status_call_ptr_       = nullptr;
  std::shared_ptr<std::mutex>   notify_mutex_ptr_   = nullptr;
  std::shared_ptr<bool>         exit_ptr_           = nullptr;
  std::shared_ptr<std::queue<NotifyData>> notify_queue_ptr_ = nullptr;
  AicCommuLogLevels log_level_ = AicCommuLogLevels::DEBUG;
  LogCall log_call_ = nullptr;       //日志回调
  bool is_log_time_ = false;         // 日志是否记录时间
};

} // namespace zmq

#endif // __AIC_COMMU_MONITOR_H__
