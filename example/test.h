#include "../src/aic_commu.h"
#include <fstream>
#include <memory>
#include <string.h>
#include <thread>
#include <vector>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define SLEEP(millisec) std::this_thread::sleep_for(std::chrono::milliseconds(millisec));

namespace aicrobot
{

bytes_ptr ReadFileBinary(std::string file_path)
{
  std::ifstream file_stream;
  file_stream.open(file_path.c_str(), std::ios::binary);

  std::filebuf *file_buf = file_stream.rdbuf();

  auto size = file_buf->pubseekoff(0, std::ios::end, std::ios::in);
  file_buf->pubseekpos(0, std::ios::in);

  bytes_ptr mem_buf = std::make_shared<bytes_vec>();
  mem_buf->resize(size);
  file_buf->sgetn(reinterpret_cast<char *>(mem_buf->data()), size);
  return mem_buf;
}

void recv_func(const std::string *p_sub,
               bytes_ptr data_in,
               bytes_ptr data_out)
{
  std::string msg(data_in->data(), data_in->data() + data_in->size());
  printf("recv ---> <content> %s ---> <size> %lu bytes\n", msg.c_str(), data_in->size());
  if (data_out != nullptr)
    data_out->assign(data_in->begin(), data_in->end());
}

void recv_req_func(const std::string *p_sub,
                   bytes_ptr data_in,
                   bytes_ptr data_out)
{
  std::string msg(data_in->data(), data_in->data() + data_in->size());
  printf("req recv ---> <content> %s ---> <size> %lu bytes\n", msg.c_str(), data_in->size());
  if (data_out != nullptr)
    data_out->assign(data_in->begin(), data_in->end());
}

bool log_func(const std::string &msg)
{
  printf("log callback ---> %s\n", msg.c_str());
  return true;
}

void status_func(AicCommuStatus status, const std::string &msg)
{
  std::string status_name;
  switch (status)
  {
  case AicCommuStatus::LISTENING:
  {
    status_name = "LISTENING";
    break;
  }
  case AicCommuStatus::BIND_FAILED:
  {
    status_name = "BIND_FAILED";
    break;
  }
  case AicCommuStatus::CONNECTED:
  {
    status_name = "CONNECTED";
    break;
  }
  case AicCommuStatus::DISCONNECTED:
  {
    status_name = "DISCONNECTED";
    break;
  }
  case AicCommuStatus::CLOSED:
  {
    status_name = "CLOSED";
    break;
  }
  case AicCommuStatus::CLOSE_FAILED:
  {
    status_name = "CLOSE_FAILED";
    break;
  }
  case AicCommuStatus::ACCEPTED:
  {
    status_name = "ACCEPTED";
    break;
  }
  case AicCommuStatus::ACCEPT_FAILED:
  {
    status_name = "ACCEPT_FAILED";
    break;
  }
  case AicCommuStatus::TIMEOUT:
  {
    status_name = "TIMEOUT";
    break;
  }
  }
  printf("status >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>%s  %s\n", status_name.c_str(), msg.c_str());
}

bool print_pack_func(bool is_send, AicCommuType type, const std::string &msg, bytes_ptr data)
{
  std::string type_str;
  switch (type)
  {
  case AicCommuType::CLIENT_REQUEST:
  {
    type_str = "request";
    break;
  }
  case AicCommuType::CLIENT_SUBSCRIBE:
  {
    type_str = "subscribe";
    break;
  }
  case AicCommuType::SERVER_REPLY:
  {
    type_str = "reply";
    break;
  }
  case AicCommuType::SERVER_PUBLISH:
  {
    type_str = "publish";
    break;
  }
  }
  std::string action = is_send ? "send" : "recv";
  printf("===========================================\n%s %s pack >>>>>> %s\n",
         type_str.c_str(),
         action.c_str(),
         msg.c_str());
  return true;
}

void *thr_req()
{
  printf("lib version:%s\n",AicCommuFactory::version().c_str());
  auto obj = AicCommuFactory::newSocket(AicCommuType::CLIENT_REQUEST, "127.0.0.1", 50005, "req");

  obj->setPollTimeout(5000);
  obj->setHeartbeatIVL(1000);
  obj->setRecvCall(&recv_func, false);
  obj->setStatusCall(&status_func);
  obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true);
  obj->setPrintPackCall(&print_pack_func);
  obj->run();
  std::string msg("request-msg");

  int i=0;
  while (true)
  {
    i++;
    char buf[100];
    sprintf(buf,"request-msg%d",i);
    bytes_ptr data = AicCommuFactory::makeBytesPtr(buf,
                                                 strlen(buf));
    bool ret = obj->send(data,nullptr,false);
    printf("%s",ret ? "-----send request success\n" : "-----send request failed\n");
    SLEEP(1000);
  }
}

void *thr_rep()
{
  auto obj = AicCommuFactory::newSocket(AicCommuType::SERVER_REPLY, "*", 50005, "rep");

  obj->setPollTimeout(5000);
  obj->setHeartbeatIVL(0); // 一般来说, 服务端不用发送心跳, 只接收
  obj->setRecvCall(&recv_func, false);
  obj->setStatusCall(&status_func);
  obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true);
  obj->setPrintPackCall(&print_pack_func);
  obj->run();

  while (true)
  {
    SLEEP(10000);
  }
}

void *thr_sub()
{
  auto obj = AicCommuFactory::newSocket(AicCommuType::CLIENT_SUBSCRIBE, "127.0.0.1", 50006, "sub");

  obj->setPollTimeout(5000);
  obj->setHeartbeatIVL(5000);
  obj->setRecvCall(&recv_func, false);
  obj->setStatusCall(&status_func);
  obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true);
  obj->setPrintPackCall(&print_pack_func);
  obj->alterSubContent("test!!", true);
  obj->run();

  while (true)
  {
    SLEEP(10000);
  }
}

void *thr_pub()
{
  auto obj = AicCommuFactory::newSocket(AicCommuType::SERVER_PUBLISH, "*", 50006, "pub");

  obj->setPollTimeout(5000);
  obj->setHeartbeatIVL(0); // 一般来说, 服务端不用发送心跳, 只接收
  obj->setStatusCall(&status_func);
  obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true);
  obj->setPrintPackCall(&print_pack_func);
  obj->run();

  std::string msg("publish-msg");
  while (true)
  {
    bytes_ptr data = std::make_shared<bytes_vec>(msg.data(),
                                                 msg.data() + msg.length());
    obj->publish("test!!", data);
    SLEEP(1000);
  }

}

} // namespace aicrobot
