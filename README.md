# aic_commu library

## 概括

aic_commu library 是一个基于 [libzmq](https://github.com/zeromq/libzmq "libzmq")  以及 [protobuf](https://github.com/protocolbuffers/protobuf "protobuf") 封装而成的通讯库。主要提供心跳、自动重连等一些网络状态检测以及恢复功能，支持请求应答、订阅发布两种模式。

通过设置回调函数避免IO堵塞、监听网络状态，数据发送使用声明的方式，通讯库根据网络状态选择性地延迟发送或超时丢弃。

****

## 快速开始

#### 依赖:

- Windows 10, Linux
- CMake 3.5 (on Linux & Windows)
- Visual Studio 2015 Update 3 (on Windows)
- C compiler and GNU C++ compiler (on Linux)
- **protobuf-3.5.1**
- **libzmq-master(commit 12005bd92629c2cca108ae1731a495e93a3aef91)**

#### 编译(linux):

1. 下载本项目代码。
2. 初始化 submodule ，获取 protobuf 和 libzmq 的源码。
3. 进入 probobuf 和 libzmq 目录，编译 protobuf 和 libzmq，将编译生成的 .a 及 .so 文件放入 lib 目录中。
4. 进入项目目录, mkdir build && cd build && cmake .. 创建 makefile。
5. make && make install 编译安装.

#### 编译(windows):

1. 下载代码,  并将项目 src/ 和 test/ 目录下所有文件编码改为 windows 默认的 ANSI 编码。
2. 进入项目目录, mkdir build && cd build && cmake .. 创建 makefile。
3. 使用 make 命令创建 visual studio solution, 用 VS IDE 打开项目。
4. 选择项目-->属性-->链接器-->常规-->附加库目录, 加入lib目录下对应debug 或 release版本的依赖库目录。
5. 编译生成 lib & dll 文件, 如果需要生成测试程序, 需将生成的 lib 文件放到步骤3的目录里, 并加入依赖关系。

#### 使用

1. 编译只需包含 src/aic_commu.h 和 libaic_commu.so (或者相应的dll) 文件。 
2. 本项目的库文件依赖于 libprotobuf.so 和 libzmq.so (或者相应的dll) 文件, 请确保环境。

# 1.1.7版本更新内容 
* 修复bug
1、修复request模式socket超时后，永远无法接收和发送数据的bug
2、修复socket析构时引起的死锁问题
* 新增功能
1、send函数添加参数，可设置为无连接时丢弃请求数据


```
Copyright (c) 2018, AicRobo.  All rights reserved.
```
