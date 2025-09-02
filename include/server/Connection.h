#pragma once
#include <memory>
#include <string>
#include <functional>
#include <event2/listener.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

// 仿照muduo的TcpServerConn封装一次连接

// enable_shared_from_this保证返回的shared_ptr与原始管理对象的shared_ptr共享同一个引用计数
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using Ptr = std::shared_ptr<Connection>;
    using OnCloseCb = std::function<void(evutil_socket_t)>;
    using OnMessageCb = std::function<void(const Ptr &, const std::string &)>;

    // 工厂函数，返回 shared_ptr，并内部设置 bufferevent 回调
    static Ptr create(bufferevent *bev, OnCloseCb onClose, OnMessageCb onMessage);

    ~Connection();

    void send(const std::string &msg);
    void close();

    bufferevent *bev() const { return _bev; }
    evutil_socket_t fd() const;

private:
    // 私有构造，通过 create 创建
    Connection(bufferevent *bev, OnCloseCb onClose, OnMessageCb onMessage);

    // libevent 回调（静态）
    static void readCb(struct bufferevent *bev, void *ctx);
    static void eventCb(struct bufferevent *bev, short events, void *ctx);

private:
    bufferevent *_bev;
    OnCloseCb _onClose;
    OnMessageCb _onMessage;
    std::weak_ptr<Connection> _self; // 用于在回调中获取 shared_ptr
};