#include "zmq_monitor_impl.h"
#include "aic_commu.h"

using namespace aicrobot;

namespace zmq
{

const std::map<uint16_t, const char *> zmq::monitor_t_impl::map_event_to_str{
    {ZMQ_EVENT_CONNECTED, "on_event_connected"},
    {ZMQ_EVENT_CONNECT_DELAYED, "on_event_connect_delayed"},
    {ZMQ_EVENT_CONNECT_RETRIED, "on_event_connect_retried"},
    {ZMQ_EVENT_LISTENING, "on_event_listening"},
    {ZMQ_EVENT_BIND_FAILED, "on_event_bind_failed"},
    {ZMQ_EVENT_ACCEPTED, "on_event_accepted"},
    {ZMQ_EVENT_ACCEPT_FAILED, "on_event_accept_failed"},
    {ZMQ_EVENT_CLOSED, "on_event_closed"},
    {ZMQ_EVENT_CLOSE_FAILED, "on_event_close_failed"},
    {ZMQ_EVENT_DISCONNECTED, "on_event_disconnected"},
};

void monitor_t_impl::invokeStatusCall(aicrobot::AicCommuStatus status, const std::string &addr,aicrobot::StatusCall func)
{
//  std::string msg_plus = aicrobot::stringFormat("[%s]", aicrobot::getDateTime(aicrobot::getTimestampNow()).c_str()) + msg;
  if (func != nullptr)
  {
    func(status, addr);
  }
}

void monitor_t_impl::socketStatusNotify(const zmq_event_t &et, const std::string &addr,aicrobot::StatusCall func){

    switch (et.event)
    {
    case ZMQ_EVENT_LISTENING:
    {
      invokeStatusCall(AicCommuStatus::LISTENING, addr,func);
      break;
    }
    case ZMQ_EVENT_BIND_FAILED:
    {
      invokeStatusCall(AicCommuStatus::BIND_FAILED, addr,func);
      break;
    }
    case ZMQ_EVENT_CONNECTED:
    {
      invokeStatusCall(AicCommuStatus::CONNECTED, addr,func);
      break;
    }
    case ZMQ_EVENT_DISCONNECTED:
    {
      invokeStatusCall(AicCommuStatus::DISCONNECTED, addr,func);
      break;
    }
    case ZMQ_EVENT_CLOSED:
    {
      invokeStatusCall(AicCommuStatus::CLOSED, addr,func);
      break;
    }
    case ZMQ_EVENT_CLOSE_FAILED:
    {
      invokeStatusCall(AicCommuStatus::CLOSE_FAILED, addr,func);
      break;
    }
    case ZMQ_EVENT_ACCEPTED:
    {
      invokeStatusCall(AicCommuStatus::ACCEPTED, addr,func);
      break;
    }
    case ZMQ_EVENT_ACCEPT_FAILED:
    {
      invokeStatusCall(AicCommuStatus::ACCEPT_FAILED, addr,func);
      break;
    }
    case ZMQ_EVENT_CONNECT_DELAYED:
    case ZMQ_EVENT_CONNECT_RETRIED:
      break;
    default:
      break;
    }
}

}
