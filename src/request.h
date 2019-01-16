#ifndef __AIC_COMMU_REQUEST_H__
#define __AIC_COMMU_REQUEST_H__
#include "aic_commu_base.h"
#include <memory>
#include <mutex>
#include <queue>
#include "waiter.h"

namespace aicrobot
{

struct ReqData // 请求数据结构
{
  bytes_ptr data_;
  RecvCall func_;

  ReqData(bytes_ptr data, RecvCall func) : data_(data), func_(func)
  {
  }
};

class AicCommuRequest : public AicCommuBase
{
public:
  AicCommuRequest(const std::string &url, const std::string &identity);
  ~AicCommuRequest() noexcept;
  virtual bool send(bytes_ptr buffer, RecvCall func = nullptr, bool discardBeforeConnected = false) override;

  virtual bool run() override;
  virtual bool close() override;
  virtual void setStatusCall(StatusCall func) override;

private:
  void createLoop();
  void clearSendQueue();
  void printPackWrapper(bool is_send, pack_ptr pack, int thread_id);
  void invokeRecvCall(bytes_ptr data);
  bool restart();

private:
//  zmq::context_t ctx_;   // 上下文环境
  zmq::socket_t socket_; // 套接字对象
  bool  is_connected_   = false;    //是否连接上服务端
  bool  is_timeout_     = false;    //是否超时
  bool  disconnect_after_timeout_    = false;    //是否是请求超时后重新连接引起的连接断开
//  bool  is_restarting_;  //正在执行restart

  std::mutex mutex_send_queue_;    // 发送队列互斥量
  std::queue<ReqData> queue_send_; // 待发送数据队列
  Waiter wait_send_queue_;         // 等待发送队列非空
};

} // namespace aicrobot

#endif // __AIC_COMMU_REQUEST_H__
