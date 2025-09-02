#include "Connection.h"
#include <event2/buffer.h>
#include <cstring>

Connection::Connection(bufferevent *bev, OnCloseCb onClose, OnMessageCb onMessage)
    : _bev(bev), _onClose(std::move(onClose)), _onMessage(std::move(onMessage))
{
    // 回调将在 create 中设置（因为需要 shared_ptr 可用）
}

Connection::~Connection()
{
    if (_bev)
    {
        bufferevent_free(_bev);
        _bev = nullptr;
    }
}

Connection::Ptr Connection::create(bufferevent *bev, OnCloseCb onClose, OnMessageCb onMessage)
{
    if (!bev)
        return nullptr;
    // 使用shared_ptr管理对象
    Ptr ptr(new Connection(bev, std::move(onClose), std::move(onMessage)));

    // 使用弱引用指针避免造成循环引用（自己引用自己）
    ptr->_self = ptr;

    // 设置回调，ctx 为裸指针（this）
    bufferevent_setcb(bev, Connection::readCb, nullptr, Connection::eventCb, ptr.get());

    // 启动读写事件监听
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    return ptr;
}

void Connection::send(const std::string &msg)
{
    if (_bev && !msg.empty())
    {
        bufferevent_write(_bev, msg.data(), msg.size());
    }
}

void Connection::close()
{
    if (_bev)
    {
        // 禁用读写回调，防止并发访问
        bufferevent_disable(_bev, EV_READ | EV_WRITE);
        // 释放底层 bufferevent
        bufferevent_free(_bev);
        _bev = nullptr;
    }
}

evutil_socket_t Connection::fd() const
{
    if (_bev)
        return bufferevent_getfd(_bev);
    return -1;
}

// 消息回调
void Connection::readCb(struct bufferevent *bev, void *ctx)
{
    // 恢复上下文（这里是指具体哪个连接）
    Connection *self = static_cast<Connection *>(ctx);
    if (!self)
        return;
    evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    if (len == 0)
        return;

    std::string msg;
    msg.resize(len);
    evbuffer_remove(input, &msg[0], len);

    // 获取 shared_ptr
    // auto sp = shared_from_this();
    // 这里不能使用裸指针。这是因为无法保证对象在执行回调前是否被删除（例如，从连接表删除），
    // 如果被删除，则self变为悬空指针，导致未定义行为！
    // shared_ptr用于延长生命周期
    auto sp = self->_self.lock();
    if (!sp)
        return;

    if (self->_onMessage)
    {
        // 交给上层处理（注意：上层处理不要阻塞）
        self->_onMessage(sp, msg);
    }
}

// 处理连接关闭、错误回调
void Connection::eventCb(struct bufferevent *bev, short events, void *ctx)
{
    Connection *self = static_cast<Connection *>(ctx);
    if (!self)
        return;

    evutil_socket_t fd = bufferevent_getfd(bev);
    bool notifyClose = false;

    // 正常关闭
    if (events & BEV_EVENT_EOF)
    {
        notifyClose = true;
    }
    else if (events & BEV_EVENT_ERROR)
    {
        // 连接出错
        notifyClose = true;
    }
    else if (events & BEV_EVENT_TIMEOUT)
    {
        // 连接超时
        notifyClose = true;
    }

    if (notifyClose)
    {
        // 通知上层清理（回调可以移除 map 中的 shared_ptr）
        if (self->_onClose)
        {
            self->_onClose(fd);
        }
        // free bufferevent 交给析构或直接释放
        if (self->_bev)
        {
            bufferevent_free(self->_bev);
            self->_bev = nullptr;
        }
    }
}