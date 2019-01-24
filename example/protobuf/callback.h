#include "../../src/aic_commu.h"
#include <fstream>
#include <memory>
#include <string.h>
#include <thread>
#include <vector>
#include "google/protobuf/text_format.h"
#include "packet.pb.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define SLEEP(millisec) std::this_thread::sleep_for(std::chrono::milliseconds(millisec));

using namespace aicrobot;
using namespace packet;

//request packet type enum
enum class RequestType{
    kTest   = 1
};

//notify packet type enum
enum class NotifyType{
    kTest   = 1
};


//打印请求包内容
#define PRINT_ENTITY_PACK(label,prefix,entity,body)  \
    std::string header_msg; \
    std::string body_msg; \
    std::string whole_msg(label); \
    body.ParseFromString(entity.body()); \
    google::protobuf::TextFormat::PrintToString(entity.header(),&header_msg); \
    google::protobuf::TextFormat::PrintToString(body,&body_msg); \
    whole_msg += prefix+"\n"+header_msg+body_msg; \
    printf("%s\n",whole_msg.data());

#define PRINT_ENTITY_HEADER(label,prefix,entity)  \
    std::string header_msg; \
    std::string whole_msg(label); \
    google::protobuf::TextFormat::PrintToString(entity.header(),&header_msg); \
    whole_msg += prefix +"\n"+ header_msg; \
    printf("%s\n",whole_msg.data());


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

  RequestData req;
  if(!req.ParseFromArray(data_in->data(),data_in->size())){
      printf("%s\n","rep_recv_func parse protobuf failed");
      return;
  }

  switch(static_cast<RequestType>(req.header().type())){
  case RequestType::kTest:
  {
      //do with test request packet
      TestRequest req_body;
      req_body.ParseFromString(req.body());

      //build response packet
      TestResponse res_body;
      res_body.set_result(req_body.msg());
      res_body.set_order(req_body.order());
      std::string res_body_str;
      res_body.SerializeToString(&res_body_str);

      ResponseData res;
      res.mutable_header()->set_err_no(1);
      res.mutable_header()->set_err_msg("SUCCESS");
      res.mutable_header()->set_type(static_cast<int>(RequestType::kTest));
      res.set_body(res_body_str);
      std::string res_str;
      res.SerializeToString(&res_str);

      //return response packet
      bytes_ptr data = std::make_shared<bytes_vec>(res_str.data(),res_str.data()+res_str.size());
      data_out->assign(data->begin(),data->end());

  }
      break;
  default:
      printf("%s\n","rep_recv_func:invalid packet type");
      break;
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
    ResponseData  res;
    if(!res.ParseFromArray(data_in->data(),data_in->size())){
        printf("%s\n","req_recv_func parse protobuf failed");
        return;
    }

    switch(static_cast<RequestType>(res.header().type())){
    case RequestType::kTest:
    {
        TestResponse res_body;
        res_body.ParseFromString(res.body());

        //TODO: do with test request packet
    }
        break;
    default:
        printf("%s\n","req_recv_func:invalid packet type");
        break;
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
    NotifyData  notify;
    if(!notify.ParseFromArray(data_in->data(),data_in->size())){
        printf("%s\n","sub_recv_func parse protobuf failed");
        return;
    }

    switch(static_cast<NotifyType>(notify.header().type())){
    case NotifyType::kTest:
    {
        TestNotify notify_body;
        notify_body.ParseFromString(notify.body());

        //TODO: do with test notify packet
    }
        break;
    default:
        printf("%s\n","invalid packet type");
        break;
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

    switch(type){
    case AicCommuType::CLIENT_REQUEST:
        if(is_send){

            RequestData req;
            if(!req.ParseFromArray(data->data(),data->size())){
                printf("%s\n","parse protobuf failed");
                return false;
            }
            switch(static_cast<RequestType>(req.header().type())){
            case RequestType::kTest:
            {
                TestRequest body;
                PRINT_ENTITY_PACK("\nSEND:\n",msg,req,body);
            }
                break;
            default:
                PRINT_ENTITY_HEADER("\nSEND:\n",msg,req);
                break;
            }

        }else{

            ResponseData res;
            if(!res.ParseFromArray(data->data(),data->size())){
                printf("%s\n","parse protobuf failed");
                return false;
            }
            switch(static_cast<RequestType>(res.header().type())){
            case RequestType::kTest:
            {
                TestResponse body;
                PRINT_ENTITY_PACK("\nRECV:\n",msg,res,body);
            }
                break;
            default:
                PRINT_ENTITY_HEADER("\nRECV:\n",msg,res);
                break;
            }

        }
        break;
    case AicCommuType::SERVER_REPLY:
        if(is_send){

            ResponseData res;
            if(!res.ParseFromArray(data->data(),data->size())){
                printf("%s\n","parse protobuf failed");
                return false;
            }
            switch(static_cast<RequestType>(res.header().type())){
            case RequestType::kTest:
            {
                TestResponse body;
                PRINT_ENTITY_PACK("\nREP:\n",msg,res,body);
            }
                break;
            default:
                PRINT_ENTITY_HEADER("\nREP:\n",msg,res);
                break;
            }

        }else{

            RequestData req;
            if(!req.ParseFromArray(data->data(),data->size())){
                printf("%s\n","parse protobuf failed");
                return false;
            }
            switch(static_cast<RequestType>(req.header().type())){
            case RequestType::kTest:
            {
                TestRequest body;
                PRINT_ENTITY_PACK("\nREQ:\n",msg,req,body);
            }
                break;
            default:
                PRINT_ENTITY_HEADER("\nREQ:\n",msg,req);
                break;
            }

        }
        break;
    case AicCommuType::CLIENT_SUBSCRIBE:
        if(!is_send){

            NotifyData notify;
            if(!notify.ParseFromArray(data->data(),data->size())){
                printf("%s\n","parse protobuf failed");
                return false;
            }
            switch(static_cast<NotifyType>(notify.header().type())){
            case NotifyType::kTest:
            {
                TestNotify body;
                PRINT_ENTITY_PACK("\nSUB:\n",msg,notify,body);
            }
                break;
            default:
                PRINT_ENTITY_HEADER("\nSUB:\n",msg,notify);
                break;
            }

        }
        break;
    case AicCommuType::SERVER_PUBLISH:
        if(is_send){

            NotifyData notify;
            if(!notify.ParseFromArray(data->data(),data->size())){
                printf("%s\n","parse protobuf failed");
                return false;
            }
            switch(static_cast<NotifyType>(notify.header().type())){
            case NotifyType::kTest:
            {
                TestNotify body;
                PRINT_ENTITY_PACK("\nPUB:\n",msg,notify,body);
            }
                break;
            default:
                PRINT_ENTITY_HEADER("\nPUB:\n",msg,notify);
                break;
            }

        }
        break;
    }

    return true;
}


