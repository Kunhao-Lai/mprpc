#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

//封装的zookeeper客户端类
class ZkClient
{
public:
    ZkClient();
    ~ZkClient();
    // zkclient启动连接zkserver
    void Start();
    //创建节点
    void Create(const char *path, const char *data, int datalen, int state = 0);
    //获取节点数据
    std::string GetData(const char *path);

private:
    // zk客户端的句柄
    zhandle_t *m_zhandle;
};