#ifndef __AIC_COMMU_H__
#define __AIC_COMMU_H__

#include <functional>
#include <memory>
#include <vector>
#include <cstring>

#ifdef _WIN32
#ifdef AIC_COMMU_BUILD
#pragma comment(lib, "libzmq.lib")
#pragma comment(lib, "libprotobuf.lib")
#define EXPORT_CLASS __declspec(dllexport)
#else
#define EXPORT_CLASS __declspec(dllimport)
#endif
#else
#define EXPORT_CLASS
#endif

#define _IN_  // 标记入参
#define _OUT_ // 标记出参


namespace aicrobot
{

//库版本号
extern std::string aic_commu_lib_version;

enum class EXPORT_CLASS AicCommuType
{
  /**
  *   与 CLIENT_REQUEST 类型成对; 接收请求, 然后发送回复。
  *   此类型的对象在回复时若与请求者的连接失效, 会静默地丢弃应答信息。
  */
  SERVER_REPLY,

  /**
  *   与 SERVER_REPLY 类型成对; 主动发起请求, 然后等待回复。
  *   此类型的对象在等待回复超时后, 会静默地丢弃待发送队列的消息。
  */
  CLIENT_REQUEST,

  /**
  *   与 CLIENT_SUBSCRIBE 类型成对; 发布信息给所有已连接上的订阅者, 不能接收消息。
  *   此类型存在待发布消息阀值(固定1000), 超过阀值的消息会被丢弃; 此类型发送信息永远不会堵塞。
  */
  SERVER_PUBLISH,

  /**
  *   与 SERVER_PUBLISH 类型成对; 接收一个发布者发布的信息, 不能发送消息。
  *   此类型初始不会订阅任何信息, 需要手动添加/删除订阅内容。
  */
  CLIENT_SUBSCRIBE,
};

enum class EXPORT_CLASS AicCommuStatus
{
  LISTENING,      // 成功监听地址
  BIND_FAILED,    // 绑定地址失败
  CONNECTED,      // 成功连接
  DISCONNECTED,   // 断开连接
  CLOSED,         // 已关闭
  CLOSE_FAILED,   // 关闭失败
  ACCEPTED,       // 成功接收连接请求
  ACCEPT_FAILED,  // 接收连接请求失败
  TIMEOUT,        // 超时(仅用于request模式，request模式下超时会自动重连，在重连过程中，不会发送DISCONNECTED事件，但会发送CONNECTED事件)
};

enum class EXPORT_CLASS AicCommuLogLevels
{
  DEBUG,          // 信息最全, 往下递减
  INFO,
  WARN,
  FATAL,
};

using bytes_vec = std::vector<unsigned char>;
using bytes_ptr = std::shared_ptr<std::vector<unsigned char>>;

/**
 * @brief LogCall         日志打印回调函数
 * @param 参数一           要打印的字符串
 * @return
 */
using LogCall = std::function<bool(const std::string &)>;

/**
 * @brief PrintPackCall   打印通讯数据回调函数
 * @param 参数一           true:发送; false:接收
 * @param 参数二           套接字类型
 * @param 参数三           附加信息
 * @param 参数四           数据内容
 * @return
 */
using PrintPackCall = std::function<bool(bool, AicCommuType, const std::string &, bytes_ptr)>;

/**
 * @brief StatusCall      状态变化回调函数
 * @param 参数一           枚举的状态类型
 * @param 参数二           与socket相关的地址信息，对于服务端socket类型，该参数是客户端的地址；对于客户端socket类型，该参数是服务端地址
 * @return
 */
using StatusCall = std::function<void(AicCommuStatus, const std::string &)>;

/**
 * @brief RecvCall        接收信息回调函数
 * @param 参数一           订阅的标识(只对 subscribe 类型有效, 否则置 nullptr)
 * @param 参数二           接收的消息
 * @param 参数三           回复的消息(只对 reply 类型有效, 否则置 nullptr)
 * @return
 */
using RecvCall = std::function<void(_IN_ const std::string *,
                                    _IN_ bytes_ptr,
                                    _OUT_ bytes_ptr)>;

/**
 * @brief AicCommuInterface 接口类
 */
class EXPORT_CLASS AicCommuInterface
{

public:
  virtual ~AicCommuInterface() = default;

  /**
   * @brief run                启动(监听或连接)
   * @return                   true:成功; false:失败
   */
  virtual bool run() = 0;

  /**
   * @brief close              关闭(监听或连接)
   * @param timeout_ms         超时时间(毫秒)
   * @return                   true:成功; false:失败或超时
   */
  virtual bool close() = 0;

  /**
   * @brief send               发送数据
   * @param buffer             发送的内容, 不能为空
   * @param func               nullptr 时使用 setRecvCall 指定的回调; 否则使用 func 指定的回调
   * @param discardBeforeConnected  该参数只有请求模式socket有效，
   *                                true:在与服务端建立连接前丢弃所有发送请求，false：不管有没有连接成功，都把发送请求加入发送队列里
   * @return                   true:已加入发送队列; false:失败
   */
  virtual bool send(bytes_ptr buffer, RecvCall func = nullptr, bool discardBeforeConnected = false) = 0;

  /**
   * @brief publish            发布消息
   * @param content            发布的标识
   * @param buffer             发布的内容, 不能为空
   * @return                   true:已加入发布队列; false:失败
   */
  virtual bool publish(const std::string &content, bytes_ptr buffer) = 0;

  /**
   * @brief setLogCall         设置打印日志回调函数
   * @param func               回调函数指针
   * @param level              设置日志级别
   * @param is_log_time        日志是否追加时间，默认false
   * @return
   */
  virtual void setLogCall(LogCall func, AicCommuLogLevels level=AicCommuLogLevels::DEBUG, bool is_log_time=false) = 0;

  /**
   * @brief setStatusCall      设置状态变化回调，必须在run之前调用，否则设置无效
   * @param func               回调函数指针
   * @return
   */
  virtual void setStatusCall(StatusCall func) = 0;

  /**
   * @brief setPrintPackCall   设置打印通讯数据回调函数
   * @param func               回调函数指针
   * @return
   */
  virtual void setPrintPackCall(PrintPackCall func) = 0;

  /**
   * @brief setRecvCall        设置接收信息回调
   * @param func               回调函数指针
   * @param is_thread_safe     true:表明func所指函数线程安全; false:线程不安全
   * @return
   */
  virtual void setRecvCall(RecvCall func, bool is_thread_safe) = 0;

  /**
   * @brief setPollTimeout     设置poll超时
   * @param milliseconds       单位毫秒, 默认值1000(不能低于该值)
   * @return
   */
  virtual void setPollTimeout(int32_t milliseconds) = 0;

  /**
   * @brief setHeartbeatIVL    设置心跳间隔(心跳超时时间为间隔两倍)
   * @param milliseconds       单位毫秒, 默认值0(表示不发送心跳, 不能低于该值)
   * @return
   */
  virtual void setHeartbeatIVL(int32_t milliseconds) = 0;

  /**
   * @brief alterSubContent    修改订阅内容(只对 subscribe 类型有效)
   * @param content            订阅内容，只能是字符串
   * @param add_or_delete      true:添加; false:删除
   * @return                   true:成功; false:失败
   */
  virtual bool alterSubContent(const std::string &content, bool add_or_delete) = 0;


};

//四种模式socket
using AICPublisher     = std::shared_ptr<AicCommuInterface>;   //推送模式socket
using AICSubscriber    = std::shared_ptr<AicCommuInterface>;   //订阅模式socket
using AICRequester     = std::shared_ptr<AicCommuInterface>;   //请求模式socket
using AICReplier       = std::shared_ptr<AicCommuInterface>;   //应答模式socket

/**
 * @brief AicCommuFactory  工厂类
 */
class EXPORT_CLASS AicCommuFactory
{
public:
  /**
   * @brief newSocket       创建 AicCommu
   * @param type            枚举值, 指定要创建的类型
   * @param ip              服务端地址; e.g."10.0.0.1", 传入"*"表示监听所有本机地址
   * @param port            服务端端口;
   * @param identity        标识字符串, 用于调试打印等
   * @return                返回接口对象的共享指针, 本库不再保留该指针的拷贝
   */
  static std::shared_ptr<AicCommuInterface> newSocket(AicCommuType type,
                                                      const std::string &ip,
                                                      const unsigned int port,
                                                      const std::string &identity);
  /**
   * @brief makeBytesPtr    构造bytes_ptr类型对象
   * @param data    数据地址
   * @param size    数据大小
   * @return 构造好的bytes_ptr对象
   */
  static inline bytes_ptr makeBytesPtr(const char* data,int size){
      bytes_ptr out = std::make_shared<bytes_vec>(size);
      memcpy(out.get()->data(),data,size);
      return out;
  }

  /**
   * @brief version 返回当前库的版本号
   * @return
   */
  static std::string version(){return aic_commu_lib_version;}
};

} // namespace aicrobot

#endif // __AIC_COMMU_H__
