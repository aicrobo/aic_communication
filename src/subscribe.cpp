#include "subscribe.h"
#include "utility.h"
#include <thread>

namespace aicrobot
{

AicCommuSubscribe::AicCommuSubscribe(const std::string &url,
                                     const std::string &identity) : socket_(AicCommuBase::ctx_, zmq::socket_type::sub)
{
  AicCommuBase::url_ = url;
  AicCommuBase::identity_ = identity;
}

AicCommuSubscribe::~AicCommuSubscribe()
{
  close();
}

bool AicCommuSubscribe::run()
{
  if (is_started_)
    return false;
  try
  {
    socket_.setsockopt(ZMQ_HEARTBEAT_IVL, heartbeat_interval_ms_);
    socket_.setsockopt(ZMQ_HEARTBEAT_TIMEOUT, heartbeat_timeout_ms_);
    socket_.setsockopt(ZMQ_RECONNECT_IVL, kReconnIVL);
    socket_.setsockopt(ZMQ_RECONNECT_IVL_MAX, kReconnMax);
    socket_.setsockopt(ZMQ_LINGER, kLingerTimeout);

    std::ostringstream o;
    o<<serial_++;
    std::string inproc_name("inproc://monitor-subscribe");
    inproc_name += o.str();

    monitor_start_waiter_->set_signaled(false);
    createMonitor(socket_, inproc_name);
    if(!monitor_start_waiter_->signaled())  //防止notify后再wait
        monitor_start_waiter_->wait();

    socket_.connect(url_);
    createLoop();
  }
  catch (zmq::error_t e)
  {
    callLog(AicCommuLogLevels::FATAL, "AicCommuSubscribe::run() throw %s err_code:%d\n",
            e.what(), e.num());
    return false;
  }
  is_started_ = true;
  return true;
}

bool AicCommuSubscribe::close()
{

  if (is_stoped_)
      return false;

  std::cout<<"enter subscribe mode socket close"<<std::endl;
  is_stoped_ = true;
  while (is_started_)
  {
    SLEEP(100);
    if (is_loop_exit_ && is_monitor_exit_)
      break;
  }
  socket_.close();
  is_started_ = false;
  std::cout<<"leave subscribe mode socket close"<<std::endl;
  return true;
}

bool AicCommuSubscribe::alterSubContent(const std::string &content,
                                        bool add_or_delete)
{
  if (content.size() == 0)
    return false;

  std::string subscriber = packSubscriber(content);
  try
  {
    if (add_or_delete)
      socket_.setsockopt(ZMQ_SUBSCRIBE, subscriber.c_str(), subscriber.size());
    else
      socket_.setsockopt(ZMQ_UNSUBSCRIBE, subscriber.c_str(), subscriber.size());
  }
  catch (zmq::error_t e)
  {
    callLog(AicCommuLogLevels::FATAL, "AicCommuSubscribe::alterSubContent() throw %s err_code:%d\n",
            e.what(), e.num());
    return false;
  }
  return true;
}

/**
 * @brief printPackWrapper      打印通讯数据包
 * @param pack                  数据包
 * @return
 */
void AicCommuSubscribe::printPackWrapper(const std::string &content, pack_ptr pack, int thread_id)
{
  if (print_pack_call != nullptr)
  {
    bytes_ptr data = std::make_shared<bytes_vec>(
        pack->mutable_data()->data(),
        pack->mutable_data()->data() + pack->data().size());
        
    std::string msg = stringFormat(
        "[tid:%d]-[content:%s] req_id:%u, identity:%s, time:%s",
        thread_id,
        content.c_str(),
        pack->req_id(),
        pack->identity().c_str(),
        std::to_string(pack->timestamp()).c_str());

    print_pack_call(false, AicCommuType::CLIENT_SUBSCRIBE, msg.c_str(), data);
  }
}

/**
 * @brief invokeRecvCall        接收回调调用
 * @param content               订阅标记
 * @param data                  数据内容
 * @return
 */
void AicCommuSubscribe::invokeRecvCall(const std::string &content, bytes_ptr data)
{
  if (recv_call_ != nullptr)
    recv_call_(&content, data, nullptr);
}



/**
 * @brief createLoop            创建主功能循环线程
 * @return
 */
void AicCommuSubscribe::createLoop()
{
  auto pkg = [&]() -> void {
    auto thread_id = getTid();
    callLog(AicCommuLogLevels::INFO, "[tid:%d] started subscribe loop.\n", thread_id);

    std::vector<zmq_pollitem_t> poll_vec;
    poll_vec.push_back({socket_, 0, ZMQ_POLLIN, 0});
    is_loop_exit_ = false;

    while (!is_stoped_)
    {
      try
      {
        // 拉取订阅数据
        zmq::poll(poll_vec, poll_timeout_ms_);
        if (poll_vec[0].revents & ZMQ_POLLIN)
        {
          zmq::message_t msg_content;
          socket_.recv(&msg_content);
          std::string content(static_cast<char *>(msg_content.data()),
                              0, msg_content.size());
          content = unpackSubscriber(content);
          zmq::message_t msg_data;
          socket_.recv(&msg_data);

          // 解析收取的 protobuf 数据包, 根据需要打印内容
          pack_ptr pack_recv = std::make_shared<pack_meta>();
          pack_recv->ParseFromArray(msg_data.data(), msg_data.size());
          printPackWrapper(content, pack_recv, thread_id);

          // 调用接收回调函数
          invokeRecvCall(content, decodeRecvBuf(pack_recv));
        }
      }
      catch (zmq::error_t e)
      {
        callLog(AicCommuLogLevels::FATAL, "[tid:%d] AicCommuSubscribe loop throw %s err_code:%d\n",
                thread_id, e.what(), e.num());
        if (e.num() == ETERM) // 上下文已终止
          break;
      }
    } // while
    callLog(AicCommuLogLevels::INFO, "[tid:%d] stoped subscribe loop.\n", thread_id);
    is_loop_exit_ = true;
  };

  std::thread thr(pkg);
  thr.detach();
}

} // namespace aicrobot
