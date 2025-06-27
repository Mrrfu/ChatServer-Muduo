#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include "json.hpp"
#include "public.hpp"
#include <functional>
#include <unordered_map>
#include "usermodel.hpp"

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;
// 一个消息ID映射一个事务处理
//  业务类，单例模式
class ChatService
{
public:
    // 静态方法，全局实例
    static ChatService *getInstance(); // 返回唯一实例接口

    void login(const TcpConnectionPtr &conn, json &js, Timestamp time); // 登录
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);   // 注册

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    ChatService();                                      // 单例模式，构造函数设为私有的
    std::unordered_map<int, MsgHandler> _msgHandlerMap; // 不同ID对应的事调函数
    UserModel _userModel;
};

#endif