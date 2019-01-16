#include "publish.h"
#include "utility.h"
#include <thread>
#include <sstream>

namespace aicrobot
{

AicCommuPublish::AicCommuPublish(const std::string &url,
                                 const std::string &identity) : socket_(AicCommuBase::ctx_, zmq::socket_type::pub)
{
  AicCommuBase::url_ = url;
  AicCommuBase::identity_ = identity;
}

AicCommuPublish::~AicCommuPublish()
{
  close();
}

bool AicCommuPublish::run()
{
  if (is_started_)
    return false;
  try
  {
    socket_.setsockopt(ZMQ_HEARTBEAT_IVL, heartbeat_interval_ms_);
    socket_.setsockopt(ZMQ_HEARTBEAT_TIMEOUT, heartbeat_timeout_ms_);
    socket_.setsockopt(ZMQ_SNDHWM, kSendQueueHWM);
    socket_.setsockopt(ZMQ_LINGER, kLingerTimeout);    

    std::ostringstream o;
    o<<serial_++;
    std::string inproc_name("inproc://monitor-publish");
    inproc_name += o.str();

    monitor_start_waiter_->set_signaled(false);
    createMonitor(socket_, inproc_name);
    if(!monitor_start_waiter_->signaled())  //防止notify后再wait
        monitor_start_waiter_->wait();

    socket_.bind(url_);
    createLoop();
  }
  catch (zmq::error_t e)
  {
    callLog(AicCommuLogLevels::FATAL, "AicCommuPublish::run() throw %s err_code:%d\n",
            e.what(), e.num());
    return false;
  }
  is_started_ = true;
  return true;
}

bool AicCommuPublish::close()
{

    if(is_stoped_)
        return false;
    std::cout<<"enter publish mode socket close"<<std::endl;
    is_stoped_ = true;
    while (is_started_)
    {
      wait_pub_queue_.broadcast();
      SLEEP(100);
      if (is_loop_exit_ && is_monitor_exit_)
        break;
    }
    is_started_ = false;
    socket_.close();
    clearPublishQueue();
    std::cout<<"leave publish mode socket close"<<std::endl;
    return true;
}

bool AicCommuPublish::publish(const std::string &content, bytes_ptr buffer)
{
  if (buffer == nullptr)
    return false;

  std::lock_guard<std::mutex> lk(mutex_pub_queue_);
  queue_pub_.emplace(content, buffer);
  wait_pub_queue_.broadcast();
  return true;
}

/**
 * @brief clearPublishQueue     清空发布队列
 * @return
 */
void AicCommuPublish::clearPublishQueue()
{
  std::lock_guard<std::mutex> lk(mutex_pub_queue_);
  while (!queue_pub_.empty())
    queue_pub_.pop();
}

/**
 * @brief printPackWrapper      打印通讯数据包
 * @param content               订阅的标记
 * @param pack                  数据包
 * @return
 */
void AicCommuPublish::printPackWrapper(const std::string &content, pack_ptr pack, int thread_id)
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

    print_pack_call(true, AicCommuType::SERVER_PUBLISH, msg.c_str(), data);
  }
}

/**
 * @brief createLoop            创建主功能循环线程
 * @return
 */
void AicCommuPublish::createLoop()
{
  auto pkg = [&]() -> void {
    auto thread_id = getTid();
    callLog(AicCommuLogLevels::INFO, "[tid:%d] started publish loop.\n", thread_id);

    is_loop_exit_ = false;
    while (!is_stoped_)
    {
      try
      {
        std::string content;
        bytes_ptr data = nullptr;
        // 从发送队列中取出一个业务数据包
        {
          std::unique_lock<std::mutex> lk(mutex_pub_queue_);
          if (queue_pub_.empty())
          {
            lk.unlock();
            wait_pub_queue_.wait();
            continue;
          }
          content = queue_pub_.front().content_;
          data = queue_pub_.front().data_;
          queue_pub_.pop();
        }

        // 先发送content
        std::string subscriber =  packSubscriber(content);
        zmq::message_t msg_content(subscriber.data(), subscriber.size());
        socket_.send(msg_content, ZMQ_SNDMORE);

        // 把业务数据包封装到 protobuf 数据里
        auto pack_send = encodeSendBuf(data, seq_id_);
        ++seq_id_;
        std::string bytes;
        pack_send->SerializeToString(&bytes);

        // 发送 protobuf 数据, 根据需要打印内容
        zmq::message_t msg_data(bytes.data(), bytes.size());
        socket_.send(msg_data);
        printPackWrapper(subscriber, pack_send, thread_id);
      }
      catch (zmq::error_t e)
      {
        callLog(AicCommuLogLevels::FATAL, "[tid:%d] AicCommuPublish loop throw %s err_code:%d\n",
                thread_id, e.what(), e.num());
        if (e.num() == ETERM) // 上下文已终止
          break;
      }
    } // while
    callLog(AicCommuLogLevels::INFO, "[tid:%d] stoped publish loop.\n", thread_id);
    is_loop_exit_ = true;
  };

  std::thread thr(pkg);
  thr.detach();
}

} // namespace aicrobot
