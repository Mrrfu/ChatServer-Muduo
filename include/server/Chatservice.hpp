#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#define ON_LINE "online"
#define OFF_LINE "offline"

#include <muduo/net/TcpConnection.h>
#include "json.hpp"
#include "public.hpp"
#include <mutex>
#include <functional>
#include <unordered_map>
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

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

    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);   // 一对一聊天
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time); // 群聊

    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time); // 加群
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void handlerRedisSubscribeMseeage(int, std::string);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 处理注销登录
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务器异常后业务重置方法
    void reset();
    //

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    ChatService();                                          // 单例模式，构造函数设为私有的
    std::unordered_map<int, MsgHandler> _msgHandlerMap;     // 不同ID对应的事调函数
    std::unordered_map<int, TcpConnectionPtr> _userConnMap; // 存储在线用户的通信连接，需考虑线程安全

    std::mutex _connMutex; // 定义互斥锁，保证_userConnMap的线程安全

    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _gruopModel;
    Redis _redis;
};

#endif