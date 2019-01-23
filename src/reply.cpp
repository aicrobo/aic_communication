#include "reply.h"
#include "utility.h"
#include <thread>

namespace aicrobot
{

AicCommuReply::AicCommuReply(const std::string &url,
                             const std::string &identity) : router_(AicCommuBase::ctx_, zmq::socket_type::router),
                                                            dealer_(AicCommuBase::ctx_, zmq::socket_type::dealer),
                                                            ctrl_pub_(AicCommuBase::ctx_, zmq::socket_type::pub),
                                                            ctrl_sub_(AicCommuBase::ctx_, zmq::socket_type::sub)
{
  AicCommuBase::url_ = url;
  AicCommuBase::identity_ = identity;
}

AicCommuReply::~AicCommuReply()
{
  close();
}

bool AicCommuReply::run()
{
  if (is_started_)
    return false;

  try
  {
    router_.setsockopt(ZMQ_HEARTBEAT_IVL, heartbeat_interval_ms_);
    router_.setsockopt(ZMQ_HEARTBEAT_TIMEOUT, heartbeat_timeout_ms_);
    router_.setsockopt(ZMQ_LINGER, kLingerTimeout);
    dealer_.setsockopt(ZMQ_HEARTBEAT_IVL, heartbeat_interval_ms_);
    dealer_.setsockopt(ZMQ_HEARTBEAT_TIMEOUT, heartbeat_timeout_ms_);
    dealer_.setsockopt(ZMQ_LINGER, kLingerTimeout);

    std::ostringstream o;
    o<<serial_++;
    std::string inproc_name("inproc://monitor-router");
    inproc_name += o.str();

    monitor_start_waiter_->set_signaled(false);
    createMonitor(router_, inproc_name);
    if(!monitor_start_waiter_->signaled())  //防止notify后再wait
        monitor_start_waiter_->wait();

    router_.bind(url_);
    std::ostringstream o2;
    o2<<serial_++;
    worker_inproc_name_ = std::string("inproc://workers");
    worker_inproc_name_ += o2.str();
    dealer_.bind(worker_inproc_name_);

    std::ostringstream o3;
    o3<<serial_++;
    std::string ctrl_inproc_name_ = std::string("inproc://ctrl");
    ctrl_inproc_name_ += o3.str();
    ctrl_pub_.bind(ctrl_inproc_name_);
    ctrl_sub_.connect(ctrl_inproc_name_);
    ctrl_sub_.setsockopt(ZMQ_SUBSCRIBE,"",0);

    createWorker();
    createProxy();
  }
  catch (zmq::error_t e)
  {
    callLog(AicCommuLogLevels::FATAL, "AicCommuReply::run() throw %s err_code:%d\n",
            e.what(), e.num());
    return false;
  }
  is_started_ = true;
  return true;
}

bool AicCommuReply::close()
{

  if(is_stoped_)
      return false;

  callLog(AicCommuLogLevels::TRACE, "%s","enter reply mode socket close\n");
  //关闭proxy
  std::string subscriber =  "TERMINATE";
  zmq::message_t msg_content(subscriber.data(), subscriber.size());
  ctrl_pub_.send(msg_content);

  is_stoped_ = true;
  while (is_started_)
  {
    SLEEP(100);
    if (is_loop_exit_ && is_monitor_exit_)
      break;
  }
  is_started_ = false;
  router_.close();
  dealer_.close();

  callLog(AicCommuLogLevels::TRACE, "%s","leave reply mode socket close\n");
  return true;
}

/**
 * @brief printPackWrapper      打印通讯数据包
 * @param is_send               true:发送; false:接收
 * @param pack                  数据包
 * @return
 */
void AicCommuReply::printPackWrapper(bool is_send, bytes_ptr pack, int thread_id)
{
  if (print_pack_call != nullptr)
  {
    char* p = pack->data();
    int total_size  = GET_INT(p);
    int header_size = GET_INT(p);
    p = pack->data();
    bytes_ptr data = std::make_shared<bytes_vec>(p+header_size , p+total_size);
    OFFSET(p,8);
    int identity_len = GET_INT(p);
    std::string identity(p,identity_len);
    OFFSET(p,identity_len);
    OFFSET(p,4);
    int pack_id = GET_INT(p);
    OFFSET(p,4);
    long long timestamp = GET_LONGLONG(p);

    std::string msg = stringFormat(
        "[tid:%d] pack_id:%u, identity:%s, time:%s",
        thread_id,
        pack_id,
        identity.c_str(),
        std::to_string(timestamp).c_str());

    print_pack_call(is_send, AicCommuType::SERVER_REPLY, msg.c_str(), data);
  }
}

/**
 * @brief invokeRecvCall        接收回调调用
 * @param data                  数据内容
 * @return
 */
bytes_ptr AicCommuReply::invokeRecvCall(bytes_ptr request_data)
{
  bytes_ptr reply_data = std::make_shared<bytes_vec>();

  if (recv_call_ != nullptr)
    recv_call_(nullptr, request_data, reply_data);

  return reply_data;
}

/**
 * @brief createProxy           创建监听代理转发线程
 * @return
 */
void AicCommuReply::createProxy()
{
  auto pkg = [&]() -> void {
    auto thread_id = getTid();
    callLog(AicCommuLogLevels::INFO, "[tid:%d] started proxy.\n", thread_id);
    try
    {
      zmq::proxy_steerable(router_, dealer_, nullptr,ctrl_sub_);
    }
    catch (zmq::error_t e)
    {
      callLog(AicCommuLogLevels::FATAL, "[tid:%d] AicCommuReply proxy throw %s err_code:%d\n",
              thread_id, e.what(), e.num());
    }
    callLog(AicCommuLogLevels::INFO, "[tid:%d] stoped proxy.\n", thread_id);
    is_loop_exit_ = true;
  };

  std::thread thr(pkg);
  thr.detach();
}

/**
 * @brief createWorker          创建请求处理线程
 * @return
 */
void AicCommuReply::createWorker()
{
  auto pkg = [&]() -> void {
    auto thread_id = getTid();
    callLog(AicCommuLogLevels::INFO, "[tid:%d] started worker.\n", thread_id);

    std::vector<zmq_pollitem_t> poll_vec;
    std::unique_ptr<zmq::socket_t> worker;
    try
    {
      worker.reset(new zmq::socket_t(ctx_, zmq::socket_type::rep));
      worker->setsockopt(ZMQ_HEARTBEAT_IVL, heartbeat_interval_ms_);
      worker->setsockopt(ZMQ_HEARTBEAT_TIMEOUT, heartbeat_timeout_ms_);
      worker->setsockopt(ZMQ_LINGER, kLingerTimeout);
      worker->connect(worker_inproc_name_);
      poll_vec.push_back({*worker, 0, ZMQ_POLLIN, 0});
    }
    catch (zmq::error_t e)
    {
      callLog(AicCommuLogLevels::FATAL, "[tid:%d] AicCommuRequest worker init throw %s err_code:%d\n",
              thread_id, e.what(), e.num());
    }

    while (!is_stoped_)
    {
      try
      {
        bytes_ptr pack_recv = nullptr;
        bytes_ptr reply_data = nullptr;
        // 拉取请求数据
        zmq::poll(poll_vec, poll_timeout_ms_); // 这里超时并不影响业务, 与request模式不同
        if (poll_vec[0].revents & ZMQ_POLLIN)
        {
          zmq::message_t msg_recv;
          worker->recv(&msg_recv);

          // 解析收取的数据包, 根据需要打印内容
          pack_recv = std::make_shared<bytes_vec>((char*)msg_recv.data(),(char*)msg_recv.data()+msg_recv.size());
          printPackWrapper(false, pack_recv, thread_id);

          // 调用接收回调函数
          reply_data = invokeRecvCall(decodeRecvBuf(pack_recv));
        }
        else
        {
          continue;
        }

        if(pack_recv == nullptr){
          callLog(AicCommuLogLevels::FATAL, "[tid:%d] recv pack failed\n",
                  thread_id);
          break;
        }

        //获取发送包的pack_id
        char* p = pack_recv->data();
        OFFSET(p,8);
        int identity_len = GET_INT(p);
        OFFSET(p,identity_len);
        OFFSET(p,4);
        int pack_id = GET_INT(p);

        // 构造完整数据包
        auto pack_send = encodeSendBuf(reply_data, pack_id);

        // 发送 protobuf 数据, 根据需要打印内容
        zmq::message_t msg_send(pack_send->data(), pack_send->size());
        worker->send(msg_send, ZMQ_NOBLOCK);
        printPackWrapper(true, pack_send, thread_id);
      }
      catch (zmq::error_t e)
      {
        callLog(AicCommuLogLevels::FATAL, "[tid:%d] AicCommuRequest worker throw %s err_code:%d\n",
                thread_id, e.what(), e.num());
        if (e.num() == ETERM) // 上下文已终止
          break;
      }
    }
    callLog(AicCommuLogLevels::INFO, "[tid:%d] stoped worker.\n", thread_id);
  };

  int thr_num = 1;
  if (is_thread_safe_recv_)
    thr_num = 4;
  for (int i = 0; i < thr_num; ++i)
  {
    std::thread thr(pkg);
    thr.detach();
  }
}

} // namespace aicrobot
