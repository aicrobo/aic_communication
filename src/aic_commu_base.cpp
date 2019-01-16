#include "aic_commu_base.h"
#include <chrono>
#include <iostream>
#include <functional>

#define SUB_SUFFIX "_aic_commu"

namespace aicrobot
{

zmq::context_t AicCommuBase::ctx_;
std::atomic<std::uint64_t> AicCommuBase::serial_(0);


AicCommuBase::AicCommuBase(){
   monitor_start_waiter_ = std::make_shared<Waiter>();
   if(AicCommuBase::serial_ == 0)
        serial_.store(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())*1000);
}

bool AicCommuBase::run()
{
  return false;
}

bool AicCommuBase::close()
{
  return false;
}

bool AicCommuBase::send(bytes_ptr buffer, RecvCall func, bool discardBeforeConnected)
{
  return false;
}

bool AicCommuBase::publish(const std::string &content, bytes_ptr buffer)
{
  return false;
}

bool AicCommuBase::alterSubContent(const std::string &content, bool add_or_delete)
{
  return false;
}

std::string AicCommuBase::packSubscriber(const std::string &content){
    std::string subscriber = content+SUB_SUFFIX;
    return subscriber;
}

std::string AicCommuBase::unpackSubscriber(const std::string &content){
    std::string subscriber = content.substr(0,content.length()-strlen(SUB_SUFFIX));
    return subscriber;
}

void AicCommuBase::createMonitor(zmq::socket_t &socket, const std::string inproc_name)
{
  auto pkg = [&, inproc_name,this]() {
    auto thread_id = getTid();
    zmq::monitor_t_impl* socket_mon = new zmq::monitor_t_impl();
    socket_mon->setStatusCall(status_call_);
    socket_mon->setLogCall(log_call_,log_level_,is_log_time_);

    try
    {
      socket_mon->init(socket, inproc_name.data(), ZMQ_EVENT_ALL);
    }
    catch (zmq::error_t e)
    {
      callLog(AicCommuLogLevels::FATAL, "[tid:%d] monitor init throw %s err_code:%d\n",
              thread_id, e.what(), e.num());
      delete socket_mon;
      return;
    }

    callLog(AicCommuLogLevels::INFO, "[tid:%d] started monitor.\n", thread_id);

    is_monitor_exit_ = false;
    bool broadcasted = false;
    while (!is_stoped_ && !is_restart_)
    {
      try
      {
        socket_mon->check_event(10);
        if(!broadcasted){
            broadcasted = true;
            monitor_start_waiter_->broadcast(); //要保证开始检测事件之后，才能让socket连接，否则会漏掉最初的一些事件
        }
      }
      catch (zmq::error_t e)
      {
        callLog(AicCommuLogLevels::FATAL, "[tid:%d] monitor throw %s err_code:%d\n",
                thread_id, e.what(), e.num());
        if (e.num() == ETERM) // 上下文已终止
          break;
      }
    }
    callLog(AicCommuLogLevels::INFO, "[tid:%d] stoped monitor.\n", thread_id);
    delete socket_mon;

    is_monitor_exit_ = true;
  }; // pkg
  std::thread thr(pkg);
  thr.detach();
}

} // namespace aicrobot
