#include "callback.h"

int main()
{

    printf("lib version:%s\n",AicCommuFactory::version().c_str());

    auto obj = AicCommuFactory::newSocket(AicCommuType::CLIENT_SUBSCRIBE, "127.0.0.1", 50006, "sub");//create communication object, essential

    obj->setPollTimeout(5000);                  //optional, set poll timeout
    obj->setHeartbeatIVL(5000);                 //optional, set heartbeat interval, hasn't heartbeat checking default
    obj->setRecvCall(&sub_recv_func, false);    //optional, set pub callback function,packet published by server will be received in this function
    obj->setStatusCall(&status_func);           //optional, set 'zmq socket' status callback function
    obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true); //optional, set log callback function of communication library
    obj->setPrintPackCall(&print_pack_func);    //optional, set data packet print callback functionl

    obj->run();                                 //essential, start up communication threads

    obj->alterSubContent("sub-test", true);     //subscribe packet

    while (true)
    {
      SLEEP(10000);
    }
    return 0;
}
