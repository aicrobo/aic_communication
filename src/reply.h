#ifndef __AIC_COMMU_REPLY_H__
#define __AIC_COMMU_REPLY_H__
#include "aic_commu_base.h"
#include <memory>

namespace aicrobot
{

class AicCommuReply : public AicCommuBase
{
public:
  AicCommuReply(const std::string &url, const std::string &identity);
  ~AicCommuReply() noexcept;

  virtual bool run() override;
  virtual bool close() override;

private:
  void createProxy();
  void createWorker();
  void printPackWrapper(bool is_send, bytes_ptr pack, int thread_id);
  bytes_ptr invokeRecvCall(bytes_ptr request_data);

private:
//  zmq::context_t ctx_;   // 上下文环境
  zmq::socket_t router_; // 负责监听外部连接
  zmq::socket_t dealer_; // 负责转发请求消息
  zmq::socket_t ctrl_sub_; // 负责接收proxy控制指令
  zmq::socket_t ctrl_pub_; // 负责发布proxy控制指令
  std::string   worker_inproc_name_;    //工作线程和dealer通信的地址
};

} // namespace aicrobot

#endif // __AIC_COMMU_REPLY_H__
