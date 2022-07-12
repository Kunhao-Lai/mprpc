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