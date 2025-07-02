#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <future>   // 确保包含了 <future> 头文件
#include <chrono>   // 确保包含了 <chrono> 头文件
#include "json.hpp" // 确保你的项目中包含了 nlohmann/json.hpp

// 使用 Boost.Asio 的 TCP 相关类型
using boost::asio::ip::tcp;

/**
 * @brief 代表一个发起登录请求的客户端
 */
class LoginClient : public std::enable_shared_from_this<LoginClient>
{
public:
    /**
     * @brief 构造函数
     * @param io io_context 的引用
     * @param endpoints 服务器的地址和端口信息
     * @param user_id 模拟的用户ID
     * @param future 用于同步启动的 shared_future
     */
    LoginClient(boost::asio::io_context &io, const tcp::resolver::results_type &endpoints, int user_id, std::shared_future<void> future)
        : io_context_(io),
          socket_(io),
          user_id_(user_id),
          start_signal_future_(future) // 保存这个 shared_future
    {
        endpoint_it_ = endpoints;
    }

    /**
     * @brief 启动客户端的异步操作流程
     * 这个函数是无阻塞的，它只是向 io_context 提交一个任务。
     */
    void start()
    {
        auto self(shared_from_this());

        // 使用 post 将任务提交到 io_context 的事件队列中，由 I/O 线程来执行
        boost::asio::post(io_context_, [this, self]()
                          {
            // 1. 等待启动信号 (这行代码会在 I/O 线程中被阻塞，直到 main 线程发出信号)
            start_signal_future_.wait();

            // 2. 信号收到后，发起异步连接
            boost::asio::async_connect(socket_, endpoint_it_,
                [this, self](const boost::system::error_code &ec, const boost::asio::ip::tcp::resolver::iterator &)
                {
                    if (!ec)
                    {
                        // 3. 连接成功后，准备并发送登录消息
                        nlohmann::json js;
                        js["msgid"] = 1;
                        js["id"] = user_id_;
                        js["password"] = "12345";
                        std::string login_msg = js.dump();

                        boost::asio::async_write(socket_, boost::asio::buffer(login_msg),
                            [this, self](const boost::system::error_code &ec, std::size_t /*bytes*/)
                            {
                                // 为避免大量输出刷屏，可以只在出错时打印日志
                                if (ec)
                                {
                                    std::cerr << "用户 " << user_id_ << " 写入错误: " << ec.message() << "\n";
                                }
                            });
                    }
                    else
                    {
                        std::cerr << "用户 " << user_id_ << " 连接失败: " << ec.message() << "\n";
                    }
                }); });
    }

private:
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    tcp::resolver::iterator endpoint_it_;
    int user_id_;
    std::shared_future<void> start_signal_future_; // 用于持有共享 future 的成员变量
};

/**
 * @brief 主函数
 */
int main(int argc, char *argv[])
{
    // 检查命令行参数
    if (argc != 4)
    {
        std::cerr << "用法: stress_client <host> <port> <number_of_users>\n";
        std::cerr << "示例: ./stress_client 127.0.0.1 8000 1000\n";
        return 1;
    }

    try
    {
        // 解析命令行参数
        const std::string host = argv[1];
        const short port = static_cast<short>(std::atoi(argv[2]));
        const int num_users = std::atoi(argv[3]);

        // 初始化 Asio
        boost::asio::io_context io;
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        // 创建用于同步的 promise 和 shared_future
        std::promise<void> start_signal;
        std::shared_future<void> start_future = start_signal.get_future().share();

        // 创建一个 work_guard，防止 io_context 在没有任务时自动退出
        // 手动控制它的生命周期
        auto work_guard = boost::asio::make_work_guard(io);

        // 创建所有客户端实例
        std::vector<std::shared_ptr<LoginClient>> clients;
        for (int i = 0; i < num_users; ++i)
        {
            auto client = std::make_shared<LoginClient>(io, endpoints, i, start_future);
            clients.push_back(client);
            client->start(); // 启动客户端，提交异步任务
        }

        // 创建并启动 I/O 线程，专门用于执行网络事件
        std::thread io_thread([&io]()
                              {
            try {
                io.run();
                std::cout << "io_context.run() 正常退出。\n";
            } catch (std::exception& e) {
                std::cerr << "io_context 异常: " << e.what() << "\n";
            } });

        // 等待用户手动触发
        std::cout << "所有客户端已创建并等待信号。按回车键触发所有登录..." << std::endl;
        std::cin.get();

        // 触发信号，让所有客户端同时开始连接
        std::cout << "正在触发登录信号...\n";
        start_signal.set_value();

        // 让测试运行一段时间
        std::cout << "测试运行中，等待15秒...\n";
        std::this_thread::sleep_for(std::chrono::seconds(15));

        // 销毁 work_guard，允许 io_context 在所有任务完成后退出
        std::cout << "重置 work_guard，允许 I/O 线程退出...\n";
        work_guard.reset();

        // 等待 I/O 线程执行完毕
        io_thread.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "异常: " << e.what() << "\n";
    }

    std::cout << "程序正常结束。\n";
    return 0;
}