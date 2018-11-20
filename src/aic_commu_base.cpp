#include "aic_commu_base.h"

#define SUB_SUFFIX "_aic_commu"

namespace aicrobot
{

bool AicCommuBase::run()
{
  return false;
}

bool AicCommuBase::close()
{
  return false;
}

bool AicCommuBase::send(bytes_ptr buffer, RecvCall func)
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
  auto pkg = [&, inproc_name]() {
    auto thread_id = getTid();
    zmq::monitor_t_impl socket_mon;

    try
    {
      socket_mon.init(socket, inproc_name.data(), ZMQ_EVENT_ALL);
    }
    catch (zmq::error_t e)
    {
      callLog(AicCommuLogLevels::FATAL, "[tid:%d] monitor init throw %s err_code:%d\n",
              thread_id, e.what(), e.num());
    }

    auto pkg = [&](const zmq_event_t &et, const std::string &addr) {
      std::string event_msg;

      if (ZMQ_EVENT_MAP.find(et.event) != ZMQ_EVENT_MAP.end())
      {
        event_msg = stringFormat("event:%s address:%s\n",
                                 ZMQ_EVENT_MAP.at(et.event),
                                 addr.c_str());
        callLog(AicCommuLogLevels::DEBUG, "[tid:%d] %s", thread_id, event_msg.c_str());
      }

      switch (et.event)
      {
      case ZMQ_EVENT_LISTENING:
      {
        invokeStatusCall(AicCommuStatus::LISTENING, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_BIND_FAILED:
      {
        invokeStatusCall(AicCommuStatus::BIND_FAILED, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_CONNECTED:
      {
        invokeStatusCall(AicCommuStatus::CONNECTED, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_DISCONNECTED:
      {
        invokeStatusCall(AicCommuStatus::DISCONNECTED, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_CLOSED:
      {
        invokeStatusCall(AicCommuStatus::CLOSED, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_CLOSE_FAILED:
      {
        invokeStatusCall(AicCommuStatus::CLOSE_FAILED, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_ACCEPTED:
      {
        invokeStatusCall(AicCommuStatus::ACCEPTED, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_ACCEPT_FAILED:
      {
        invokeStatusCall(AicCommuStatus::ACCEPT_FAILED, event_msg.c_str());
        break;
      }
      case ZMQ_EVENT_CONNECT_DELAYED:
      case ZMQ_EVENT_CONNECT_RETRIED:
        break;
      default:
        callLog(AicCommuLogLevels::INFO, "[tid:%d] on_event_unknown code:%u address:%s\n",
                thread_id, et.event, addr.c_str());
        break;
      }
    }; // pkg

    socket_mon.setNotify(pkg);
    callLog(AicCommuLogLevels::INFO, "[tid:%d] started monitor.\n", thread_id);
    while (!is_stoped_)
    {
      try
      {
        socket_mon.check_event(1000);
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
    is_monitor_exit_ = true;
  }; // pkg
  std::thread thr(pkg);
  thr.detach();
}

} // namespace aicrobot
