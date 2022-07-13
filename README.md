# mprpc
一个基于muduo库的rpc分布式微服务框架

## 概述
1. 使用muduo开源网络库和Protobuf数据序列化和反序列化协议，实现了高效的RPC通信。
2. 使用互斥量和条件变量实现线程安全的缓冲队列，实现了异步的日志输出。
3. 使用服务器中间件ZooKeeper的服务注册和服务发现机制，实现了分布式下的服务发现功能。 

## 基本框架


## 预编译环境
1. 安装protobuf
2. 安装zookeeper
3. 安装muduo库
4. 安装CMake集成编译环境


## 项目目录
mprpc-------bin                 可执行文件
       |----build               项目编译文件
       |----lib                 项目库文件
       |----src                 源文件
             |------include     头文件
             |------logger.cc               日志类
             |------mprpcapplication.cc     框架类
             |------mprpcchannel.cc         服务使用类
             |------rpcprovider.cc          服务提供类
             |------mprpcconfig.cc          配置类
             |------zookeeperutil.cc        zookeeper类
       |----test                测试代码
       |----example             框架使用范例
       |----CMakeLists.txt      顶层的CMake文件
       |----autobuild.sh        一键编译脚本