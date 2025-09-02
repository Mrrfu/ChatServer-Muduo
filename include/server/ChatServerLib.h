#ifndef CHATSERVERLIB_H
#define CHATSERVERLIB_H

#include <event2/event.h>
#include <event2/listener.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include "Connection.h"
#include "Logger.h"


/*基于libevent网络库*/

class ChatServerLib
{
public:
    ChatServerLib(const std::string &ip, uint16_t port, const std::string &nameArg);
    ~ChatServerLib();
    void start();

private:
    static void acceptConnCb(evconnlistener *listener, evutil_socket_t fd,
                             sockaddr *addr, int socklen, void *ctx);

    // 成员
    struct event_base *_base;
    struct evconnlistener *_listener;
    std::string _name;
    std::string _ip;
    uint16_t _port;

    std::unordered_map<evutil_socket_t, Connection::Ptr> _connMap;
    std::mutex _connMutex;
    Logger logger;
};

#endif