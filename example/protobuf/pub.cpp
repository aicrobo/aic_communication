#include "callback.h"

void publishSimulation(std::shared_ptr<AicCommuInterface> obj){

    int order = 1;
    while (true)
    {
      //build data packet
      TestNotify body;
      body.set_msg("test pub msg");
      body.set_order(order++);
      std::string body_str;
      body.SerializeToString(&body_str);
      NotifyData req;
      req.mutable_header()->set_type(static_cast<int>(RequestType::kTest));
      req.set_body(body_str);
      std::string pack;
      req.SerializeToString(&pack);

      bytes_ptr data = std::make_shared<bytes_vec>(pack.data(),pack.data() + pack.length());

      obj->publish("sub-test", data);
      SLEEP(1000);
    }

}

int main()
{
    printf("lib version:%s\n",AicCommuFactory::version().c_str());

    auto obj = AicCommuFactory::newSocket(AicCommuType::SERVER_PUBLISH, "*", 50006, "pub");//create communication object, essential

    obj->setPollTimeout(5000);              //optional, set poll timeout
    obj->setHeartbeatIVL(0);                //optional, set heartbeat interval, hasn't heartbeat checking default
    obj->setStatusCall(&status_func);       //optional, set 'zmq socket' status callback function
    obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true); //optional, set log callback function of communication library
    obj->setPrintPackCall(&print_pack_func);//optional, set data packet print callback functionl
    obj->run();                             //essential, start up communication threads

    publishSimulation(obj);
    return 0;
}
