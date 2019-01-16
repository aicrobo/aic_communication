#ifndef __AIC_COMMU_PUBLISH_H__
#define __AIC_COMMU_PUBLISH_H__
#include "aic_commu_base.h"
#include <memory>
#include <mutex>
#include <queue>
#include "waiter.h"

namespace aicrobot
{

struct PubData // 发布数据结构
{
  std::string content_;
  bytes_ptr data_;

  PubData(const std::string &content, bytes_ptr data) :
  content_(content), data_(data)
  {
  }
};

class AicCommuPublish : public AicCommuBase
{
public:
  AicCommuPublish(const std::string &url, const std::string &identity);
  ~AicCommuPublish() noexcept;
  virtual bool publish(const std::string &content, bytes_ptr buffer) override;

  virtual bool run() override;
  virtual bool close() override;

private:
  void createLoop();
  void clearPublishQueue();
  void printPackWrapper(const std::string &content, pack_ptr pack, int thread_id);

private:
//  zmq::context_t ctx_;            // 上下文环境
  zmq::socket_t socket_;          // 套接字对象
  Waiter wait_pub_queue_;         // 等待发布队列非空
  std::mutex mutex_pub_queue_;    // 发布队列互斥量
  std::queue<PubData> queue_pub_; // 待发布数据队列
  const int kSendQueueHWM = 1000; // 待发布数据最大数量
};

} // namespace aicrobot

#endif // __AIC_COMMU_PUBLISH_H__
