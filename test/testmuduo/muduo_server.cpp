
/*
muduo网络库给用户提供了两个主要的类
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace muduo;
using namespace muduo::net;

/*
 *基于muduo网络库开发服务器程序
 *1.组合TcpServer对象
 *2.创建EventLoop时间循环对象指针
 *3.明确TcpServer构造函数需要什么对象，输出ChatServer的构造函数
 *4.在当前服务器类的构造函数中设置处理连接和读写的回调函数
 *5.设置合适的服务端线程数，muduo库会自动分配
 */
class ChatServer
{
public:
    // loop:事件循环，lisetenAddr:IP地址+端口，nameAdr服务器名字
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
    {

        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
        // 给服务器注册用户读写时间回调
        _server.setMessageCallback(std::bind(&ChatServer::onMseeage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // 设置服务器端的线程数（1个IO线程数，5个工作线程）
        _server.setThreadNum(6);
    }
    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门处理连接用户的连接和断开
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
            std::cout << conn->peerAddress().toIpPort() << "->"
                      << conn->localAddress().toIpPort() << " state:online" << std::endl;
        else
        {
            std::cout << conn->peerAddress().toIpPort() << "->"
                      << conn->localAddress().toIpPort() << " state:offline" << std::endl;
            conn->shutdown();
        }
    }
    void onMseeage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 时间信息
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data: " << buf << " time" << time.toFormattedString() << std::endl;
        conn->send(buf);
    }
    TcpServer _server;
    EventLoop *_loop;
};

int main()
{
    int a=0;
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop(); // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写时间等
    return 0;
}