#include "reply.h"
#include "utility.h"
#include <thread>

namespace aicrobot
{

AicCommuReply::AicCommuReply(const std::string &url,
                             const std::string &identity) : router_(ctx_, zmq::socket_type::router),
                                                            dealer_(ctx_, zmq::socket_type::dealer)
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

    createMonitor(router_, "inproc://monitor-router");
    router_.bind(url_);
    dealer_.bind("inproc://workers");

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
  if (!is_stoped_)
  {
    is_stoped_ = true;
    router_.close();
    dealer_.close();
    ctx_.close();
  }
  while (true)
  {
    SLEEP(100);
    if (is_loop_exit_ && is_monitor_exit_)
      break;
  }
  return true;
}

/**
 * @brief printPackWrapper      打印通讯数据包
 * @param is_send               true:发送; false:接收
 * @param pack                  数据包
 * @return
 */
void AicCommuReply::printPackWrapper(bool is_send, pack_ptr pack, int thread_id)
{
  if (print_pack_call != nullptr)
  {
    bytes_ptr data = std::make_shared<bytes_vec>(
        pack->mutable_data()->data(),
        pack->mutable_data()->data() + pack->data().size());

    std::string msg = stringFormat(
        "[tid:%d] req_id:%u, identity:%s, time:%s",
        thread_id,
        pack->req_id(),
        pack->identity().c_str(),
        std::to_string(pack->timestamp()).c_str());

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
      zmq::proxy(router_, dealer_, nullptr);
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
      worker->connect("inproc://workers");
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
        pack_ptr pack_recv = nullptr;
        bytes_ptr reply_data = nullptr;
        // 拉取请求数据
        zmq::poll(poll_vec, poll_timeout_ms_); // 这里超时并不影响业务, 与request模式不同
        if (poll_vec[0].revents & ZMQ_POLLIN)
        {
          zmq::message_t msg_recv;
          worker->recv(&msg_recv);

          // 解析收取的 protobuf 数据包, 根据需要打印内容
          pack_recv = std::make_shared<pack_meta>();
          pack_recv->ParseFromArray(msg_recv.data(), msg_recv.size());
          printPackWrapper(false, pack_recv, thread_id);

          // 调用接收回调函数
          reply_data = invokeRecvCall(decodeRecvBuf(pack_recv));
        }
        else
        {
          continue;
        }

        // 把业务数据包封装到 protobuf 数据里
        auto pack_send = encodeSendBuf(reply_data, pack_recv->req_id());
        std::string bytes;
        pack_send->SerializeToString(&bytes);

        // 发送 protobuf 数据, 根据需要打印内容
        zmq::message_t msg_send(bytes.data(), bytes.size());
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