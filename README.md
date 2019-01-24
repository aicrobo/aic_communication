# aic_commu library

## Description


Aic_commu library is a communication library based on  [libzmq](https://github.com/zeromq/libzmq "libzmq"). It mainly provides some network status detection and recovery functions, such as heartbeat detection, disconnection or automatic reconnection of heartbeat timeout. It supports two modes: request response and subscription publishing.


Using this library is simpler than programming directly with zmq, shielding many underlying details, and transferring data between business layer and communication library through callback function, so that callers only care about business logic.


****

## QUICK START

* request client
```c
auto obj = AicCommuFactory::newSocket(AicCommuType::CLIENT_REQUEST, "127.0.0.1", 60005, "req");
obj->setRecvCall(&req_recv_func, false);
obj->run(); 
sting msg = "test req msg";
bytes_ptr pack = std::make_shared<bytes_vec>(msg.data(),msg.data()+msg.size()); 
obj->send();
```

* response server
```c
auto obj = AicCommuFactory::newSocket(AicCommuType::SERVER_REPLY, "*", 60005, "rep");
obj->setRecvCall(&rep_recv_func, false);
obj->run(); 
```

* subscribe client
```c
auto obj = AicCommuFactory::newSocket(AicCommuType::CLIENT_SUBSCRIBE, "127.0.0.1", 60006, "sub");
obj->setRecvCall(&sub_recv_func, false);
obj->run();
obj->alterSubContent("sub-test", true);
```

* publish server
```c
auto obj = AicCommuFactory::newSocket(AicCommuType::SERVER_PUBLISH, "*", 60006, "pub");
obj->run();
std::string msg      = "test pub msg";
bytes_ptr pack = std::make_shared<bytes_vec>(msg.data(),msg.data() + msg.length());
obj->publish("sub-test", pack);
```

#### DEPENDS:

- Windows 10, Linux
- CMake 3.5 (on Linux & Windows)
- Visual Studio 2015 Update 3 (on Windows)
- C compiler and GNU C++ compiler (on Linux)
- **libzmq-master(commit 12005bd92629c2cca108ae1731a495e93a3aef91)**

#### BUILD(linux):

1. download aic_communication source code。
2. download libzmq source code
    libzmq（https://github.com/zeromq/libzmq.git）
3. build zmq to static library
```c
cd libzmq
./autogen.sh
./configure --enable-static --with-pic 
make
sudo make install
```
4. build aic_communication
```c
cd  aic_communication
mkdir build 
cd build 
cmake .. 
make
sudo make install
```
	
5. build example
 * in step 4,replace cmake .. with
    - cmake .. -DTEST_JSON=YES (only build json version example )
    - cmake .. -DTEST_PROTOBUF=YES (only build protobuf version example)
    - cmake .. -DTEST=YES (build all version example)

#### BUILD(windows):

1. download aic_communication source code。
2. download libzmq source code
	libzmq（https://github.com/zeromq/libzmq.git）
3. build zmq
```c
cd  aic_communication/builds
mkdir windows 
cd windows 
cmake ../.. 
double click libzmq.vcxproj，build dll and lib
```
4. build aic_communication
```c
cd aic_communication
mkdir build
cmake ..
double click aic_commu.sln，right click aic_commu project，click property，config header and lib file paths of zmq and protobuf，build aic_commu.dll and aic_commu.lib。
```
5. build example
 * in step 4,replace cmake .. with
    - cmake .. -DTEST_JSON=YES (only build json version example )
    - cmake .. -DTEST_PROTOBUF=YES (only build protobuf version example)
    - cmake .. -DTEST=YES (build all version example)
 * notice: if you build protobuf version example,you must download and build [protobuf](https://github.com/protocolbuffers/protobuf "protobuf") first, then run commands:'cd example/protobuf' 'protoc --cpp_out=. packet.proto'.then you will get two new files:packet.pb.h,packet.pb.cc

#### USING

prepare：config right ips in source code,rebuild project 

request-reply mode

 * open one console，run req_json or req_proto
 * open another console，run rep_json or rep_proto
 
publish-subscribe mode

 * open one console，run sub_json or sub_proto
 * open another console，run pub_json or pub_proto

notice: *_json and *_json is a pair,*_proto and *_proto is a pair,you can't run one *_json and one *_proto 



### -----------VERSION LOG-----------

#### 1.2 version   
* update:
 * remove libprotobuf dependence 

#### 1.1.7 version   

* fix bugs:
 * fix bug what you never can send and recv data after request timeout when in request mode。
 * fix bug what deadlock caused by socket destruction。

* new functions:
 * add param to send function，it will discard send packet when connect with server hasn't build。



Copyright (c) 2018, AicRobo.  All rights reserved.
