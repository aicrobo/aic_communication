#include "callback.h"

void publishSimulation(std::shared_ptr<AicCommuInterface> obj){

    int order = 1;
    while (true)
    {
      //build publish packet
      json pack;
      pack["header"]["type"]    = 1;
      pack["body"]["msg"]       = "test pub msg";
      pack["body"]["order"]     = order++;
      std::string pack_str      = pack.dump();
      bytes_ptr data = std::make_shared<bytes_vec>(pack_str.data(),pack_str.data() + pack_str.length());

      obj->publish("sub-test", data);
      SLEEP(1000);
    }

}

int main()
{
    printf("lib version:%s\n",AicCommuFactory::version().c_str());

    auto obj = AicCommuFactory::newSocket(AicCommuType::SERVER_PUBLISH, "*", 60006, "pub");//create communication object, essential

    obj->setPollTimeout(5000);              //optional, set poll timeout
    obj->setHeartbeatIVL(0);                //optional, set heartbeat interval, hasn't heartbeat checking default
    obj->setStatusCall(&status_func);       //optional, set 'zmq socket' status callback function
    obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true); //optional, set log callback function of communication library
    obj->setPrintPackCall(&print_pack_func);//optional, set data packet print callback functionl
    obj->run();                             //essential, start up communication threads

    publishSimulation(obj);
    return 0;
}
