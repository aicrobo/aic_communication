# aic_commu library

## 概括

aic_commu library 是一个基于 [libzmq](https://github.com/zeromq/libzmq "libzmq")  封装而成的通讯库。主要提供心跳检测、断线或心跳超时自动重连等一些网络状态检测以及恢复功能，支持请求应答、订阅发布两种模式。

使用该库比直接使用zmq编程更加简单，屏蔽掉了很多底层细节，通过回调函数在业务层与通信库传递数据，使调用者只用关心业务逻辑。


****

## 快速开始

#### 依赖:

- Windows 10, Linux
- CMake 3.5 (on Linux & Windows)
- Visual Studio 2015 Update 3 (on Windows)
- C compiler and GNU C++ compiler (on Linux)
- **libzmq-master(commit 12005bd92629c2cca108ae1731a495e93a3aef91)**

#### 编译(linux):

1. 下载本项目代码。
2. 下载依赖库源码
	libzmq（https://github.com/zeromq/libzmq.git）
3. 编译zmq为静态库
```c
cd libzmq
./autogen.sh
./configure --enable-static --with-pic 
make
sudo make install
```
4. 编译aic_communication
```c
cd  aic_communication
mkdir build 
cd build 
cmake .. 
make
sudo make install
```	

#### 编译(windows):

1. 下载本项目代码。
2. 下载依赖库源码
	libzmq（https://github.com/zeromq/libzmq.git）
3. 编译zmq
```c
cd  aic_communication/builds
mkdir windows 
cd windows 
cmake ../.. 
双击打开libzmq.vcxproj，生成dll与lib
```
4. 编译aic_communication
```c
cd aic_communication
mkdir build
cmake ..
双击打开aic_commu.sln，右键aic_commu项目，点击属性，配置好zmq和protobuf的头文件与库文件目录，生成aic_commu.dll与aic_commu.lib。
配置pub、rep、req、sub四个工程的库文件目录，生成可执行文件
```


#### 使用

准备：设置好实例程序中的ip地址，重新编译实例程序。

请求应答模式

 * 开启一个终端，启动req
 * 开启另一个终端，启动rep
 
发布订阅模式

 * 开启一个终端，启动sub
 * 开启另一个终端，启动pub

## -----------版本记录-----------
## 1.1.7版本更新内容 
修复bug。
 * 修复request模式socket超时后，永远无法接收和发送数据的bug。
 * 修复socket析构时引起的死锁问题。

新增功能。
 * send函数添加参数，可设置为无连接时丢弃请求数据。



Copyright (c) 2018, AicRobo.  All rights reserved.
