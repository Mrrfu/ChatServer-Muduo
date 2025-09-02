#include "ChatServerLib.h"
#include "ChatServiceLib.h"
#include "../thirdparty/json.hpp"
#include <event2/buffer.h>
#include <arpa/inet.h>
#include <iostream>
#include "Logger.h"

using json = nlohmann::json;

ChatServerLib::ChatServerLib(const std::string &ip, uint16_t port, const std::string &nameArg)
    : _base(event_base_new()), _listener(nullptr), _name(nameArg), _ip(ip), _port(port), logger("ServerLogger")
{
    // event_base 是事件循环句柄，管理事件分发与I/O

    // 构造IPV4地址结构
    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &sin.sin_addr);

    // 在给定的event_base绑定监听器
    _listener = evconnlistener_new_bind(_base,
                                        acceptConnCb, // 连接回调
                                        this,         // 回调的上下文指针
                                        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                        -1,
                                        (sockaddr *)&sin,
                                        sizeof(sin));
    if (!_listener)
    {
        std::cerr << " evconnlistener_new_bind failed\n";
    }
}

ChatServerLib::~ChatServerLib()
{
    if (_listener)
    {
        evconnlistener_free(_listener);
    }
    if (_base)
    {
        event_base_free(_base);
    }
}

void ChatServerLib::start()
{
    // 开启事件调度循环
    LOG(INFO) << "Server start,ip: " << _ip << " port: " << _port;
    event_base_dispatch(_base);
}

/*
 * 静态回调函数，当libevent监听器接收到新连接请求时被调用，主要功能是：
 * （1）为新连接创建bufferevent
 *  (2)设置处理和关闭连接回调
 * （3）创建Connection对象封装连接
 * （4）将连接存入连接表以维护其生命周期
 */

void ChatServerLib::acceptConnCb(evconnlistener *listener, evutil_socket_t fd,
                                 sockaddr *addr, int socklen, void *ctx)
{
    ChatServerLib *server = static_cast<ChatServerLib *>(ctx);
    bufferevent *bev = bufferevent_socket_new(server->_base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev)
    {
        std::cerr << "create bufferevent error!\n";
        evutil_closesocket(fd);
        return;
    }
    LOG(INFO) << "new connection,fd: " << fd;
    // onMessage 回调：解析字符串并分发，这里直接解析并未考虑TCP的粘包和拆包问题
    auto onMessage = [server](const Connection::Ptr &conn, const std::string &raw)
    {
        try
        {
            json js = json::parse(raw);
            auto handler = ChatServiceLib::getInstance()->getHandler(js["msgid"].get<int>());
            handler(conn, js, Timestamp::now());
        }
        catch (const std::exception &e)
        {
            // 解析失败，忽略或记录
        }
    };

    // onClose 回调：移除连接表
    auto onClose = [server](evutil_socket_t cfd)
    {
        std::lock_guard<std::mutex> lk(server->_connMutex);
        auto it = server->_connMap.find(cfd);
        if (it != server->_connMap.end())
        {
            LOG(INFO) << "fd: " << cfd << " disconnected!";
            server->_connMap.erase(it);
        }
    };

    // 创建 Connection 并保存
    auto conn = Connection::create(bev, onClose, onMessage);
    if (!conn)
    {
        evutil_closesocket(fd);
        return;
    }

    // 加入连接表以维护生命周期
    {
        std::lock_guard<std::mutex> lk(server->_connMutex);
        server->_connMap[fd] = conn;
    }
}