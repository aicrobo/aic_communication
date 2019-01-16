#ifndef __AIC_COMMU_BASE_H__
#define __AIC_COMMU_BASE_H__
#include "aic_commu.h"
#include "aic_commu.pb.h"
#include "utility.h"
#include "zmq.hpp"
#include "zmq_monitor_impl.h"
#include <mutex>
#include <atomic>
#include <chrono>
#include "waiter.h"


namespace aicrobot
{

using pack_meta = aic_commu_proto::message_pack; // protobuf 的类
using pack_ptr = std::shared_ptr<aic_commu_proto::message_pack>;

/**
 * @brief AicCommuBase  基类
 */
class AicCommuBase : public AicCommuInterface
{
public:
  AicCommuBase();
  virtual ~AicCommuBase() noexcept = default;
  AicCommuBase(const AicCommuBase &) = delete;
  AicCommuBase &operator=(const AicCommuBase &) = delete;
  AicCommuBase(AicCommuBase &&) = delete;
  AicCommuBase &operator=(AicCommuBase &&) = delete;

  virtual bool run() override;
  virtual bool close() override;
  virtual bool send(bytes_ptr buffer, RecvCall func = nullptr, bool discardBeforeConnected = false) override;
  virtual bool publish(const std::string &content, bytes_ptr buffer) override;
  virtual bool alterSubContent(const std::string &content, bool add_or_delete) override;

  virtual void setLogCall(LogCall func, AicCommuLogLevels level, bool is_log_time) override
  {
    log_call_ = func;
    log_level_ = level;
    is_log_time_ = is_log_time;
  }

  virtual void setStatusCall(StatusCall func) override
  {
    status_call_ = func;
  }

  virtual void setPrintPackCall(PrintPackCall func) override
  {
    print_pack_call = func;
  }

  virtual void setRecvCall(RecvCall func, bool is_thread_safe) override
  {
    recv_call_ = func;
    is_thread_safe_recv_ = is_thread_safe;
  }

  virtual void setPollTimeout(int32_t milliseconds) override
  {
    if (milliseconds > 1000)
      poll_timeout_ms_ = milliseconds;
  }

  virtual void setHeartbeatIVL(int32_t milliseconds) override
  {
    if (milliseconds > 0)
    {
      heartbeat_interval_ms_ = milliseconds;
      heartbeat_timeout_ms_ = milliseconds * 2;
    }
  }

protected:
  /**
   * @brief callLog       格式化日志, 并转发回调函数
   * @param level              日志级别
   * @param format             格式化规则
   * @param args               格式化参数
   * @return
   */
  template <typename... Args>
  void callLog(AicCommuLogLevels level, const std::string &format, Args... args)
  {
    if (static_cast<int>(level) < static_cast<int>(log_level_))
      return;
    std::string msg = stringFormat(format, args...);
    if (is_log_time_)
      msg = stringFormat("[%s]", getDateTime(getTimestampNow()).c_str()) + msg;

    if (log_call_ != nullptr)
      log_call_(msg);
    else
      printf("%s", msg.c_str());
  }

  /**
   * @brief encodeSendBuf      封装 protobuf 数据包
   * @param buffer             数据内容
   * @param seq_id             序列号
   * @return                   shared_ptr 包裹的 protobuf 对象
   */
  pack_ptr encodeSendBuf(const bytes_ptr &buffer, int32_t seq_id)
  {
    pack_ptr pack = std::make_shared<pack_meta>();
    pack->set_req_id(seq_id);
    pack->set_identity(identity_);
    pack->set_timestamp(getTimestampNow());
    pack->set_data(static_cast<const void *>(buffer->data()), buffer->size());
    return pack;
  }

  /**
   * @brief decodeRecvBuf      解析接收的数据内容
   * @param pack               shared_ptr 包裹的 protobuf 对象
   * @return                   数据内容
   */
  bytes_ptr decodeRecvBuf(const pack_ptr &pack)
  {
    int size = pack->data().size();
    bytes_ptr data = std::make_shared<bytes_vec>(
        pack->mutable_data()->data(),
        pack->mutable_data()->data() + size);
    return data;
  }

  /**
   * @brief invokeStatusCall   状态回调的简单封装
   * @param status             状态类型枚举
   * @param msg                辅助信息，一般为 peer 的地址信息
   * @return
   */
  void invokeStatusCall(AicCommuStatus status, const std::string &msg)
  {
    std::lock_guard<std::mutex> lk(mutex_status_call_);
    std::string msg_plus = stringFormat("[%s]", getDateTime(getTimestampNow()).c_str()) + msg;
    if (status_call_ != nullptr)
    {
      status_call_(status, msg_plus);
    }
  }


  /**
   * @brief createMonitor      创建套接子监控线程
   * @param socket             被监控的套接子
   * @param inproc_name        监控用的管道名
   * @param is_client          true: 客户端; false:服务端
   * @return
   */
  void createMonitor(zmq::socket_t &socket, const std::string inproc_name);

  /**
   * @brief packSubscriber 在content前面加上后缀生成订阅类型，因为zmq比对订阅字符串不是完全相等
   * @param content
   * @return
   */
  std::string packSubscriber(const std::string &content);

  /**
   * @brief packSubscriber 把订阅类型字符串的后缀去掉
   * @param content
   * @return
   */
  std::string unpackSubscriber(const std::string &content);

protected:
  static zmq::context_t ctx_;   // 上下文环境
  static std::atomic<std::uint64_t> serial_;

  std::string url_;      // 根据socket类型用于监听或连接
  std::string identity_; // 标识字符串, 用于调试打印等


  int32_t seq_id_ = 1; // 数据包序列号

  int32_t poll_timeout_ms_ = 1 * 1000; // poll超时(毫秒)
  int32_t heartbeat_timeout_ms_ = 0;   // 心跳超时时间(毫秒)
  int32_t heartbeat_interval_ms_ = 0;  // 心跳间隔时间(毫秒)

  LogCall log_call_ = nullptr;             // 打印日志回调函数
  RecvCall recv_call_ = nullptr;           // 接收数据回调函数
  StatusCall status_call_ = nullptr;       // 状态变化回调函数
  PrintPackCall print_pack_call = nullptr; // 打印通讯数据回调函数

  AicCommuLogLevels log_level_ = AicCommuLogLevels::DEBUG;  // 日志级别
  std::mutex mutex_status_call_; // 状态变化互斥量

  WaiterPtr monitor_start_waiter_;  //等待监控socket启动

  bool is_log_time_ = false;         // 日志是否记录时间
  bool is_thread_safe_recv_ = false; // recv_call_ 是否线程安全

  bool is_monitor_exit_ = false; // 仅当对象被销毁时设置为true
  bool is_loop_exit_ = false;    // 仅当对象被销毁时设置为true
  bool is_started_ = false;      // 标记当前socket是否已经启动
  bool is_stoped_ = false;       // 仅当对象被销毁时设置为true
  bool is_restart_ = false;      // 仅当request类型socket请求超时重新时设置为true

  const int kReconnIVL = 4 * 1000; // 重连初始周期(毫秒)
  const int kReconnMax = 8 * 1000; // 重连周期最大值(毫秒)
  const int kLingerTimeout = 0;    // 关闭链接后堵塞时间(毫秒)
};

} // namespace aicrobot

#endif // __AIC_COMMU_BASE_H__
