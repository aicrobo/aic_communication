#include "request.h"
#include "utility.h"
#include <thread>
#include <iostream>

namespace aicrobot
{

AicCommuRequest::AicCommuRequest(const std::string &url,
                                 const std::string &identity) : socket_(AicCommuBase::ctx_, zmq::socket_type::req)
{
  AicCommuBase::url_ = url;
  AicCommuBase::identity_ = identity;
  poll_timeout_ms_ = 5 * 1000;  //默认请求超时时间
}

AicCommuRequest::~AicCommuRequest()
{
  close();
}

bool AicCommuRequest::run()
{
  if (is_started_ || is_stoped_)
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
    std::string inproc_name("inproc://monitor-request");
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
    callLog(AicCommuLogLevels::FATAL, "AicCommuRequest::run() throw %s err_code:%d\n",
            e.what(), e.num());
    return false;
  }
  is_started_ = true;
  return true;
}

/**
 * @brief AicCommuRequest::restart  清除旧的连接，建立新的连接
 * @return
 */
bool AicCommuRequest::restart(){

    //释放旧资源
    is_restart_ = true;
    while (true)
    {
      wait_send_queue_.broadcast();
      SLEEP(100);
      if (is_monitor_exit_)
        break;
    }
    socket_.close();
    if(discard_packet_before_connect_){
        std::cout<<"\n=========clear send queue\n";
        clearSendQueue();
    }


    //创建新的socket
    try{
       socket_ = zmq::socket_t(AicCommuBase::ctx_,zmq::socket_type::req);
    }catch(zmq::error_t e){
        std::cout<<"create req socket failed: "<<e.what()<<std::endl;
        callLog(AicCommuLogLevels::FATAL,"create req socket failed: %s",e.what());
        is_restart_ = false;
        return false;
    }

    is_restart_ = false;
    is_started_ = false;
    bool ret = run();
    if(ret)
        is_monitor_exit_ = false;

    return ret;

}

bool AicCommuRequest::close()
{

  if (is_stoped_)
    return false;

  std::cout<<"enter request mode socket close"<<std::endl;
  is_stoped_ = true;
  auto start = std::chrono::system_clock::now();
  while (is_started_)
  {
    if (is_loop_exit_ && is_monitor_exit_)
      break;
    wait_send_queue_.broadcast();
    SLEEP(100);
  }

  socket_.close();
  clearSendQueue();
  is_started_ = false;
  std::cout<<"leave request mode socket close"<<std::endl;
  return true;
}

/**
 * @brief AicCommuRequest::setStatusCall    设置通讯状态回调，更新连接标志
 * @param func
 * @return
 */
void AicCommuRequest::setStatusCall(StatusCall func){
    if(func == nullptr){
        status_call_ = nullptr;
    }else{
        status_call_ = [func,this](AicCommuStatus status, const std::string &msg){
            if(status == AicCommuStatus::CONNECTED){
                this->is_connected_ = true;
//                if(is_timeout_){    //由超时引起的重新连接不再通知调用者
//                    is_timeout_ = false;
//                    return;
//                }
            }else if(status == AicCommuStatus::DISCONNECTED ){
                this->is_connected_ = false;
                if(is_timeout_ && disconnect_after_timeout_){    //由超时重连引起的连接断开不再通知调用者
                    disconnect_after_timeout_ = false;
                    return;
                }
            }else if(status == AicCommuStatus::TIMEOUT){
                is_timeout_ = true;
                disconnect_after_timeout_ = true;
            }

            if(is_stoped_){ //对象將要释放时，忽略掉任何通知
                return;
            }

            func(status,msg);
        };
    }
}

void AicCommuRequest::setDiscardPacketBeforeConntect(bool param)
{
    discard_packet_before_connect_ = param;
}

bool AicCommuRequest::send(bytes_ptr buffer, RecvCall func, bool discardBeforeConnected)
{
  if (buffer == nullptr)
    return false;

  if(discardBeforeConnected && is_connected_ == false){
      callLog(AicCommuLogLevels::INFO,"%s","discard send request before connected");
      return false; //连接成功前丢充掉所有请求
  }

  bool is_empty = false;
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
    callLog(AicCommuLogLevels::INFO, "[tid:%d] started request loop.\n", thread_id);

    std::vector<zmq_pollitem_t> poll_vec;
    poll_vec.push_back({socket_, 0, ZMQ_POLLIN, 0});

    bool is_timeout = false;
    ReqData req_data(nullptr, nullptr);
    is_loop_exit_ = false;

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
          queue_send_.pop();

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
          invokeStatusCall(AicCommuStatus::TIMEOUT, url_);
          break;

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
    callLog(AicCommuLogLevels::INFO, "[tid:%d] stoped request loop.\n", thread_id);


    if(is_timeout && !is_stoped_){
        callLog(AicCommuLogLevels::DEBUG, "[tid:%d] reconnect after timeout.\n", thread_id);
        if(!restart())  //超时后，重新创建socket连接
             is_loop_exit_ = true;
    }else
        is_loop_exit_ = true;


  };

  std::thread thr(pkg);
  thr.detach();
}

} // namespace aicrobot
