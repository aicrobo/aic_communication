#include "zmq_monitor_impl.h"

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

}
