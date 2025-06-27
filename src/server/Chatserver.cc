#include "Chatserver.hpp"
#include "Chatservice.hpp"
#include "../thirdparty/json.hpp"
#include <string>
using namespace std;
using namespace ::placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    _server.setThreadNum(6); // CPU为6核心
}
void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 用户断开连接
    if (!conn->connected())
    {
        conn->shutdown();
    }
}
// 上报读写相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp receiveTime)
{
    // 收到消息
    std::string buf = buffer->retrieveAllAsString();

    // 数据反序列化
    json js = json::parse(buf);

    // 解耦网络模块的代码和业务模块的代码
    //  通过js["msgid"]获取---->业务handler
    auto msgHandler = ChatService::getInstance()->getHandler(js["msgid"].get<int>()); // 获取对应handler

    msgHandler(conn, js, receiveTime); // 执行
}