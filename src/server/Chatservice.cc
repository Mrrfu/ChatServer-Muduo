#include "Chatservice.hpp"
#include <muduo/base/Logging.h>
#include <string>
using namespace muduo;

ChatService *ChatService::getInstance()
{
    static ChatService service;
    return &service;
}
ChatService::ChatService()
{
    // _msgHandlerMap
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this,
                                                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this,
                                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
}
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个异常，打印日志
        return [msgid](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do login service!";
}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do reg service!";
}