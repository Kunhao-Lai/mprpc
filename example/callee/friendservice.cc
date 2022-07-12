#include <iostream>
#include <string>
#include <vector>

#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"

using namespace fixbug;

class FriendService : public fixbug::FriendServiceRpc
{
public:
    std::vector<std::string> GetFriendList(uint32_t userid)
    {
        std::cout << "do GetFriendList service"
                  << "userid" << userid << std::endl;
        std::vector<std::string> vec;
        vec.emplace_back("zhang san");
        vec.emplace_back("li si");
        vec.emplace_back("wang wu");
        return vec;
    }

    void GetFriendLists(google::protobuf::RpcController *controller,
                        const fixbug::GetFriendListsRequest *request,
                        fixbug::GetFriendListsResponse *response,
                        google::protobuf::Closure *done)
    {
        //获取请求，进行本地业务调用
        uint32_t userid = request->userid();

        //本地方法调用
        std::vector<std::string> friendList = GetFriendList(userid);

        //响应写入response
        ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        for (auto &name : friendList)
        {
            std::string *p = response->add_friends();
            *p = name;
        }
        //执行回调操作
        done->Run();
    }
};

int main(int argc, char **argv)
{
    LOG_INFO("first log message!");
    LOG_ERR("%s:%d", __FILE__, __LINE__);
    // 1. 框架的初始化
    MprpcApplication::Init(argc, argv);

    // 2. 把UserService对象发布到rpc节点上,RpcProvider是一个rpc网络服务对象
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 3.启动一个rpc服务发布节点
    provider.Run();

    return 0;
}