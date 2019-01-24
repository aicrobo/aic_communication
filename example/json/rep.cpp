#include "callback.h"

int main()
{

    printf("lib version:%s\n",AicCommuFactory::version().c_str());

    auto obj = AicCommuFactory::newSocket(AicCommuType::SERVER_REPLY, "*", 60005, "rep");//create communication object, essential

    obj->setPollTimeout(5000);                  //optional, set poll timeout, 1 seconds default
    obj->setHeartbeatIVL(0);                    //optional, set heartbeat interval, hasn't heartbeat checking default
    obj->setRecvCall(&rep_recv_func, false);    //essential, set request callback function, packet sended by client will be received in this function
    obj->setStatusCall(&status_func);           //optional, set 'zmq socket' status callback function
    obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true); //optional, set log callback function of communication library
    obj->setPrintPackCall(&print_pack_func);    //optional, set data packet print callback functionl
    obj->run();                                 //essential, start up communication threads

    while (true)
    {
      SLEEP(10000);
    }

    getchar();
    return 0;
}
