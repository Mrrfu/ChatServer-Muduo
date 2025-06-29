#include "Chatservice.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <map>
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
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this,
                                                   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this,
                                                     std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this,
                                                       std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this,
                                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this,
                                                     std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
}

void ChatService::reset()
{
    // 把所有online用户设置为offline
    _userModel.reset();
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
    int id = js["id"].get<int>();
    std::string pwd = js["password"];

    User user = _userModel.query(id);

    if (user.getId() == id && user.getPassword() == pwd)
    {
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录！";
            conn->send(response.dump());
        }
        else
        {
            {
                // 登录成功，记录用户连接信息
                std::lock_guard<std::mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 修改用户在线状态
            user.setState("online");
            _userModel.updateState(user);

            json response;

            // 获取离线消息
            std::vector<std::string> vec = _offlineMsgModel.query(user.getId());
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取后删除离线消息
                _offlineMsgModel.remove(user.getId());
            }

            // 查询该用户的好友信息并返回
            std::vector<User> friends;
            friends = _friendModel.query(user.getId());
            if (!friends.empty())
            {
                std::vector<std::string> temp;
                for (User &user : friends)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    temp.emplace_back(js.dump());
                }
                response["friends"] = temp;
            }
            // 查询群组列表
            std::vector<Group> groups;
            groups = _gruopModel.queryGroups(user.getId());
            if (!groups.empty())
            {
                std::vector<std::string> groupinfo;
                for (Group &group : groups)
                {
                    json js;
                    js["groupid"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();

                    // 查询当前群组的成员信息
                    std::vector<std::string> userinfo;
                    for (GroupUser &groupuser : group.getUsers())
                    {
                        json js;
                        js["id"] = groupuser.getId();
                        js["name"] = groupuser.getName();
                        js["state"] = groupuser.getState();
                        js["role"] = groupuser.getRole();
                        userinfo.push_back(js.dump());
                    }

                    js["users"] = userinfo;
                    groupinfo.push_back(js.dump());
                }
                response["groups"] = groupinfo;
            }

            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名不存在或者密码错误！";
        conn->send(response.dump());
    }

    // 查询到了就登录并修改在线状态
}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    std::string name = js["name"];
    std::string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息
            it->second->send(js.dump());
            return;
        }
    }

    // 用户不在线,上传离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 定义消息的json格式
    //   id: 发消息id，groupid：当前所在群id，msgid：消息id，name:发消息用户名
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 返回当前群组的所有用户（除了自己）
    std::vector<int> userids = _gruopModel.queryGroupUsers(userid, groupid);
    // for (auto id : userids)
    //     LOG_INFO << id;
    // 发送消息
    std::lock_guard<std::mutex> lock(_connMutex); // 上锁
    for (const int &id : userids)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 在线，转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 存储离线消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}

// msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int freiednid = js["friendid"].get<int>();
    // 验证friendid

    // 写入friendID
    _friendModel.insert(userid, freiednid);
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];
    Group group(-1, name, desc);

    // 1.创建群组
    if (_gruopModel.createGroup(group))
    {
        // 2.将自身设置为管理员
        _gruopModel.addGroup(userid, group.getId(), "creator");
    }
    else
        LOG_INFO << "创建群组失败！";
}

// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _gruopModel.addGroup(userid, groupid, "normal");
}

// 处理用户异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
    else
    {
        LOG_INFO << "未查找到相应连接用户！";
    }
}
