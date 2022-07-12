#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

/*  service_name  =>  service描述
 *                        =>  service* 记录服务对象
 *                        method_name => method方法对象
 */
//发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;

    //获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    //获取服务的名字
    std::string service_name = pserviceDesc->name();
    //获取服务对象service的方法数量
    int methodCnt = pserviceDesc->method_count();

    // std::cout << "service_name: " << service_name << std::endl;
    LOG_INFO("service_name: %s", service_name.c_str());
    for (int i = 0; i < methodCnt; ++i)
    {
        //获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        LOG_INFO("method_name: %s", method_name.c_str());
        // std::cout << "method_name: " << method_name << std::endl;
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

//启动rpc服务节点，提供rpc远程网络调用服务
void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");

    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    //创建TcpServer
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    //绑定连接回调和消息读写回调方法  分离网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));

    //设置muduo库的线程数量
    server.setThreadNum(4);

    //把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    ZkClient zkCli;
    zkCli.Start();

    for (auto &sp : m_serviceMap)
    {
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);

            //创建一个临时性节点（ZOO_EPHEMERAL）
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    std::cout << "RpcProvider start service at ip: " << ip << " port: " << port << std::endl;

    //启动网络服务
    server.start();
    m_eventLoop.loop();
}
//连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        //连接断开
        conn->shutdown();
    }
}
/*
框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
service_name method_name args   定义proto的message类型，进行数据的序列化和反序列化

header_size(4) + header_str + args_str
header_str: service_name + method_name + args_size
*/
//已建立连接用户的读写事件回调
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp)
{
    //网络模块接收的远程rpc调用请求的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    //读取前四个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char *)&header_size, 4, 0);

    //根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);

    mprpc::RpcHeader rpcHeader;

    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        //数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        //数据头反序列化失败
        std::cout << "rpc_header_str: " << rpc_header_str << " parse error!" << std::endl;
        return;
    }

    //获取参数信息
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    //打印调试信息
    std::cout << "===============================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_size: " << args_size << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "===============================" << std::endl;

    //获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        std::cout << service_name << "is not exist" << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << " : " << method_name << "is not exist" << std::endl;
        return;
    }

    //获取service对象
    google::protobuf::Service *service = it->second.m_service;
    //获取method对象
    const google::protobuf::MethodDescriptor *method = mit->second;

    //生成rpc方法调用的请求和响应参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        std::cout << "request parse error,content: " << args_str << std::endl;
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    /*
    void UserServiceRpc::CallMethod(const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method,
                                 ::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                                 const ::PROTOBUF_NAMESPACE_ID::Message* request,
                                 ::PROTOBUF_NAMESPACE_ID::Message* response,
                                 ::google::protobuf::Closure* done)
    */

    // 给下面method方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                                                    const muduo::net::TcpConnectionPtr &,
                                                                    google::protobuf::Message *>(this,
                                                                                                 &RpcProvider::SendRpcResponse,
                                                                                                 conn,
                                                                                                 response);

    //在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;

    if (response->SerializeToString(&response_str)) // response进行序列化
    {
        //序列化成功后，通过网络把rpc方法执行的结果发送给rpc的调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl;
    }

    //断开连接(短连接，一次调用完成之后就断开)
    conn->shutdown();
}