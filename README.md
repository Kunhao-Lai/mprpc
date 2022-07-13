# mprpc
一个基于muduo库的rpc分布式微服务框架

## 概述
1. 使用muduo开源网络库和Protobuf数据序列化和反序列化协议，实现了高效的RPC通信。
2. 使用互斥量和条件变量实现线程安全的缓冲队列，实现了异步的日志输出。
3. 使用服务器中间件ZooKeeper的服务注册和服务发现机制，实现了分布式下的服务发现功能。 


## 基本框架
<div align="center">
  <img src="https://github.com/Kunhao-Lai/mprpc/blob/main/mprpc.png">
</div>


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
       
## 使用示例
### 调用端
```
#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    //整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定要先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    //调用远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    
    // rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    // rpc方法的响应
    fixbug::LoginResponse response;
    //发起rpc方法的调用  同步rpc的调用过程
    stub.Login(nullptr, &request, &response, nullptr); // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    //一次rpc调用完成，读调用的结果
    if (response.result().errcode() == 0)
    {
        std::cout << "rpc login response success: " << response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error: " << response.result().errmsg() << std::endl;
    }

    // rpc方法的请求参数
    fixbug::RegisterRequest registerRequest;
    registerRequest.set_id(2);
    registerRequest.set_name("zhang san");
    registerRequest.set_pwd("123456");
    // rpc方法的响应
    fixbug::RegisterResponse registerResponse;
    //发起rpc方法的调用  同步rpc的调用过程
    stub.Register(nullptr, &registerRequest, &registerResponse, nullptr); // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    //一次rpc调用完成，读调用的结果
    if (registerResponse.result().errcode() == 0)
    {
        std::cout << "rpc register response success: " << registerResponse.success() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error: " << registerResponse.result().errmsg() << std::endl;
    }
    return 0;
}
```
### 服务提供端
```
#include <iostream>
#include <string>

#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

using namespace fixbug;
/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
*/
class UserService : public UserServiceRpc // Rpc服务的提供者
{
public:
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name: " << name << "pwd: " << pwd << std::endl;
        return true;
    }
    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service: Register" << std::endl;
        std::cout << "id: " << id << "name: " << name << "pwd: " << pwd << std::endl;
        return true;
    }
    //重写基类UserServiceRpc的虚函数 下面这些方法由框架直接调用
    void Register(::google::protobuf::RpcController *controller,
                  const ::fixbug::RegisterRequest *request,
                  ::fixbug::RegisterResponse *response,
                  ::google::protobuf::Closure *done)
    {
        //获取请求，进行本地业务调用
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        //本地方法调用
        bool register_result = Register(id, name, pwd);

        //响应写入response
        ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(register_result);

        //执行回调操作
        done->Run();
    }

    void Login(::google::protobuf::RpcController *controller,
               const ::fixbug::LoginRequest *request,
               ::fixbug::LoginResponse *response,
               ::google::protobuf::Closure *done)
    {
        //获取请求，进行本地业务调用
        std::string name = request->name();
        std::string pwd = request->pwd();

        //本地方法调用
        bool login_result = Login(name, pwd);

        //响应写入response
        ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //执行回调操作
        done->Run();
    }
};

int main(int argc, char **argv)
{
    // 1. 框架的初始化
    MprpcApplication::Init(argc, argv);

    // 2. 把UserService对象发布到rpc节点上,RpcProvider是一个rpc网络服务对象
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 3.启动一个rpc服务发布节点
    provider.Run();

    return 0;
}
```
