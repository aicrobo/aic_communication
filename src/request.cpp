#include "request.h"
#include "utility.h"
#include <thread>

namespace aicrobot
{

AicCommuRequest::AicCommuRequest(const std::string &url,
                                 const std::string &identity) : socket_(ctx_, zmq::socket_type::req)
{
  AicCommuBase::url_ = url;
  AicCommuBase::identity_ = identity;
}

AicCommuRequest::~AicCommuRequest()
{
  close();
}

bool AicCommuRequest::run()
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

    createMonitor(socket_, "inproc://monitor-request");
    socket_.connect(url_);
    createLoop();
  }
  catch (zmq::error_t e)
  {
    callLog(AicCommuLogLevels::FATAL, "AicCommuRequest::run() throw %s err_code:%d\n",
            e.what(), e.num());
    return false;
  }
  is_started_ = true;
  return true;
}

bool AicCommuRequest::close()
{
  if (!is_stoped_)
  {
    is_stoped_ = true;
    socket_.close();
    ctx_.close();
    clearSendQueue();
  }
  while (true)
  {
    wait_send_queue_.broadcast();
    SLEEP(100);
    if (is_loop_exit_ && is_monitor_exit_)
      break;
  }
  return true;
}

bool AicCommuRequest::send(bytes_ptr buffer, RecvCall func)
{
  if (buffer == nullptr)
    return false;
  bool is_empty;
  {
    std::lock_guard<std::mutex> lk(mutex_send_queue_);
    is_empty = queue_send_.empty();
    queue_send_.emplace(buffer, func);
  }
  if (is_empty)
    wait_send_queue_.broadcast();
  return true;
}

/**
 * @brief clearSendQueue     清空发送队列
 * @return
 */
void AicCommuRequest::clearSendQueue()
{
  std::lock_guard<std::mutex> lk(mutex_send_queue_);
  while (!queue_send_.empty())
    queue_send_.pop();
}

/**
 * @brief printPackWrapper      打印通讯数据包
 * @param is_send               true:发送; false:接收
 * @param pack                  数据包
 * @return
 */
void AicCommuRequest::printPackWrapper(bool is_send, pack_ptr pack, int thread_id)
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

    print_pack_call(is_send, AicCommuType::CLIENT_REQUEST, msg.c_str(), data);
  }
}

/**
 * @brief invokeRecvCall        接收回调调用
 * @param data                  数据内容
 * @return
 */
void AicCommuRequest::invokeRecvCall(bytes_ptr data)
{
  if (recv_call_ != nullptr)
    recv_call_(nullptr, data, nullptr);
}

/**
 * @brief createLoop            创建主功能循环线程
 * @return
 */
void AicCommuRequest::createLoop()
{
  auto pkg = [&]() -> void {
    auto thread_id = getTid();
    callLog(AicCommuLogLevels::INFO, "[tid:%d] started loop.\n", thread_id);

    std::vector<zmq_pollitem_t> poll_vec;
    poll_vec.push_back({socket_, 0, ZMQ_POLLIN, 0});

    bool is_timeout = false;
    ReqData req_data(nullptr, nullptr);

    while (!is_stoped_)
    {
      try
      {
        if(!is_timeout)
        {
          // 从发送队列中取出一个业务数据包
          {
            std::unique_lock<std::mutex> lk(mutex_send_queue_);
            if (queue_send_.empty())
            {
              lk.unlock();
              wait_send_queue_.wait();
              continue;
            }
            req_data = queue_send_.front();
            queue_send_.pop();
          }
          // 把业务数据包封装到 protobuf 数据里
          auto pack_send = encodeSendBuf(req_data.data_, seq_id_);
          ++seq_id_;
          std::string bytes;
          pack_send->SerializeToString(&bytes);

          // 发送 protobuf 数据, 根据需要打印内容
          zmq::message_t msg_send(bytes.data(), bytes.size());
          socket_.send(msg_send);
          printPackWrapper(true, pack_send, thread_id);
        }

        // 拉取应答数据
        zmq::poll(poll_vec, poll_timeout_ms_);
        if (poll_vec[0].revents & ZMQ_POLLIN)
        {
          zmq::message_t msg_recv;
          socket_.recv(&msg_recv);

          // 解析收取的 protobuf 数据包, 根据需要打印内容
          pack_ptr pack_recv = std::make_shared<pack_meta>();
          pack_recv->ParseFromArray(msg_recv.data(), msg_recv.size());
          printPackWrapper(false, pack_recv, thread_id);

          // 调用接收回调函数
          if (req_data.func_ != nullptr)
            req_data.func_(nullptr, decodeRecvBuf(pack_recv), nullptr);
          else
            invokeRecvCall(decodeRecvBuf(pack_recv));
          is_timeout = false;
        }
        else
        {
          // 发送的数据应答超时, 通知调用方
          is_timeout = true;
          invokeStatusCall(AicCommuStatus::TIMEOUT, "request timeout");
        }
      }
      catch (zmq::error_t e)
      {
        callLog(AicCommuLogLevels::FATAL, "[tid:%d] AicCommuRequest loop throw %s err_code:%d\n",
                thread_id, e.what(), e.num());
        if (e.num() == ETERM) // 上下文已终止
          break;
      }
    } // while
    callLog(AicCommuLogLevels::INFO, "[tid:%d] stoped loop.\n", thread_id);
    is_loop_exit_ = true;
  };

  std::thread thr(pkg);
  thr.detach();
}

} // namespace aicrobot
