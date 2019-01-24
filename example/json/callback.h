#include "../../src/aic_commu.h"
#include <fstream>
#include <memory>
#include <string.h>
#include <thread>
#include <vector>
#include "json.hpp"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define SLEEP(millisec) std::this_thread::sleep_for(std::chrono::milliseconds(millisec));

using namespace aicrobot;
using json = nlohmann::json;


/**
 * @brief rep_recv_func recv function of reply socket
 * @param p_sub         ignore
 * @param data_in       request packet sended by client
 * @param data_out      response packet will be send to client
 */
void rep_recv_func(const std::string *p_sub,
               bytes_ptr data_in,
               bytes_ptr data_out)
{
  std::string msg(data_in->data(), data_in->data() + data_in->size());

  try{
      json pack = json::parse(msg);
      if (data_out != nullptr){

          //build response packet
          json res;
          res["header"]["err_no"]   = 1;
          res["header"]["err_msg"]  = "SUCCESS";
          res["body"]["result"]     = pack["body"]["msg"];
          res["body"]["order"]      = pack["body"]["order"];
          std::string res_str = res.dump();
          bytes_ptr data = std::make_shared<bytes_vec>(res_str.data(),res_str.data()+res_str.size());

          //return response packet,aic_commu will send it to client
          data_out->assign(data->begin(),data->end());
      }
  }catch(std::exception e){
      printf("%s",e.what());
  }

}

/**
 * @brief req_recv_func     recv funcion of request socket
 * @param p_sub             ignore
 * @param data_in           recvied data packet
 * @param data_out          ignore
 */
void req_recv_func(const std::string *p_sub,
               bytes_ptr data_in,
               bytes_ptr data_out)
{
  std::string msg(data_in->data(), data_in->data() + data_in->size());

  try{
      json pack = json::parse(msg);

      //TODO: do with response packet

  }catch(std::exception e){
      printf("%s",e.what());
  }

}


/**
 * @brief req_recv_func     recv funcion of subscribe socket
 * @param p_sub             subscribe identity string
 * @param data_in           recvied data packet
 * @param data_out          ignore
 */
void sub_recv_func(const std::string *p_sub,
               bytes_ptr data_in,
               bytes_ptr data_out)
{
  std::string msg(data_in->data(), data_in->data() + data_in->size());

  try{

      if(p_sub->compare("sub-test") == 0){

          json pack = json::parse(msg);

          //TODO: do with subscribed packet
      }

  }catch(std::exception e){
      printf("%s",e.what());
  }

}


bool log_func(const std::string &msg)
{
  printf("aic_commu log callback ---> %s\n", msg.c_str());
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

    printf("\n%s\n",msg.c_str());
    std::string json_str(data->data(), data->data() + data->size());

    switch(type){
    case AicCommuType::CLIENT_REQUEST:
        if(is_send){
            printf("SEND:\n%s\n",json_str.c_str());
        }else{
            printf("RECV:\n%s\n",json_str.c_str());
        }
        break;
    case AicCommuType::SERVER_REPLY:
        if(is_send){
            printf("REP:\n%s\n",json_str.c_str());
        }else{
            printf("REQ:\n%s\n",json_str.c_str());
        }
        break;
    case AicCommuType::CLIENT_SUBSCRIBE:
        if(!is_send){
            printf("SUB:\n%s\n",json_str.c_str());
        }
        break;
    case AicCommuType::SERVER_PUBLISH:
        if(is_send){
            printf("PUB:\n%s\n",json_str.c_str());
        }
        break;
    }

    return true;
}


