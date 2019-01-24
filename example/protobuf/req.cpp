#include "callback.h"

void requestSimulation(std::shared_ptr<AicCommuInterface> obj){

    unsigned int order = 1;
    while(1){

        //build data packet
        TestRequest body;
        body.set_msg("test req msg");
        body.set_order(order++);
        std::string body_str;
        body.SerializeToString(&body_str);
        RequestData req;
        req.mutable_header()->set_type(1);
        req.set_body(body_str);
        std::string pack;
        req.SerializeToString(&pack);

        bytes_ptr data = std::make_shared<bytes_vec>(pack.data(), pack.data()+pack.size());

        /**method1  - send data packet，the recv packet will be returned in callback function set by 'setRecvCall',
                if the connection with server hasn't build,then the packet will be pushed into the send queue,
                it will be send to the server later when
                the connection is build
        */
        obj->send(data);

        /**method2  - same behaviour with method1,the only difference is that the packet will be discarded if server is no connected
        obj->send(data,nullptr,true);
         */

        /**method3  - send data packet，the recv packet will be returned in the lambda expression
        obj->send(data,[](const std::string *p_sub, bytes_ptr data_in, bytes_ptr data_out){
            //do with the returned packet
        });
        */

        SLEEP(1000);
    }

}

int main()
{

    printf("lib version:%s\n",AicCommuFactory::version().c_str());

    auto obj = AicCommuFactory::newSocket(AicCommuType::CLIENT_REQUEST, "127.0.0.1", 50005, "req");//create communication object, essential

    obj->setPollTimeout(5000);                  //optional, set request timeout, default 5 seconds
    obj->setHeartbeatIVL(1000);                 //optional, set heartbeat interval, hasn't heartbeat checking default
    obj->setRecvCall(&req_recv_func, false);    //optional, set response callback function,packet returned by server will be received in this function
    obj->setStatusCall(&status_func);           //optional, set 'zmq socket' status callback function
    obj->setLogCall(&log_func, AicCommuLogLevels::DEBUG, true); //optional, set log callback function of communication library
    obj->setPrintPackCall(&print_pack_func);    //optional, set data packet print callback functionl

    obj->run();                                 //essential, start up communication threads

    requestSimulation(obj);
    return 0;
}

