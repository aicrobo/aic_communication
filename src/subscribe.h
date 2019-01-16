#ifndef __AIC_COMMU_SUBSCRIBE_H__
#define __AIC_COMMU_SUBSCRIBE_H__
#include "aic_commu_base.h"
#include <memory>

namespace aicrobot
{

class AicCommuSubscribe : public AicCommuBase
{
public:
  AicCommuSubscribe(const std::string &url, const std::string &identity);
  ~AicCommuSubscribe() noexcept;
  virtual bool alterSubContent(const std::string &content, bool add_or_delete) override;

  virtual bool run() override;
  virtual bool close() override;

private:
  void createLoop();
  void printPackWrapper(const std::string &content, pack_ptr pack, int thread_id);
  void invokeRecvCall(const std::string &content, bytes_ptr data);

private:
//  zmq::context_t ctx_;   // 上下文环境
  zmq::socket_t socket_; // 套接字对象
};

} // namespace aicrobot

#endif // __AIC_COMMU_SUBSCRIBE_H__
