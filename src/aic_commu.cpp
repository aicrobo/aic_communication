#include "aic_commu.h"
#include "aic_commu_base.h"
#include "request.h"
#include "reply.h"
#include "subscribe.h"
#include "publish.h"
#include <iostream>

namespace aicrobot
{

std::string aic_commu_lib_version = "1.1.7";

std::shared_ptr<AicCommuInterface> AicCommuFactory::newSocket(
    AicCommuType type,
    const std::string &ip,
    const unsigned int port,
    const std::string &identity)
{
  std::shared_ptr<AicCommuInterface> obj = nullptr;
  std::string url = "tcp://" + ip + ":" + std::to_string(port);
  try{
      if (AicCommuType::CLIENT_REQUEST == type)
        obj = std::make_shared<AicCommuRequest>(url, identity);
      if (AicCommuType::SERVER_REPLY == type)
        obj = std::make_shared<AicCommuReply>(url, identity);
      if (AicCommuType::CLIENT_SUBSCRIBE == type)
        obj = std::make_shared<AicCommuSubscribe>(url, identity);
      if (AicCommuType::SERVER_PUBLISH == type)
        obj = std::make_shared<AicCommuPublish>(url, identity);
  }catch(zmq::error_t e){
      std::cout<< "create socket failed: "<<e.what()<<" errnum:"<<e.num()<<std::endl;
  }

  return obj;
}

}
