#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include "json.hpp"
using boost::asio::ip::tcp;

void chat_client(int id)
{
    try
    {
        boost::asio::io_context io;
        tcp::socket s(io);
        s.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 8000));
        nlohmann::json js;
        js["msgid"] = 1;
        js["id"] = id;
        js["password"] = "12345";
        std::string msg = js.dump();

        boost::asio::write(s, boost::asio::buffer(msg));
        // 让线程不退出
        while (1)
        {
        }

        // char reply[4096];
        // size_t reply_length = boost::asio::read(s, boost::asio::buffer(reply));

        // std::cout << "Client " << id << " received: " << std::string(reply, reply_length) << std::endl;
    }
    catch (const boost::system::system_error &e)
    {
        std::cerr << "[ERROR] Client " << id << " failed: " << e.what()
                  << " (code=" << e.code().value() << ", msg=" << e.code().message() << ")"
                  << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] Client " << id << " exception: " << e.what() << std::endl;
    }
}

int main()
{
    const int client_count = 80000; // 测试并发数量
    std::vector<std::thread> clients;

    for (int i = 0; i < client_count; ++i)
    {
        std::cout << i << std::endl;
        clients.emplace_back(chat_client, i);
    }

    for (auto &t : clients)
    {
        if (t.joinable())
            t.join();
    }

    return 0;
}
