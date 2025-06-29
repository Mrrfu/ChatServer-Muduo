#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <chrono>
#include <ctime>
#include <unordered_map>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"
#include "json.hpp"

using namespace std;

using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;

// 记录用户好友列表信息
vector<User> g_currentUserFriendList;

// 记录当前用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 显示用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间
string getCurrentTime();

// 主聊天界面
void mainMenu(int clientid);

// 主线程用作发送线程，子线程作为接受线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 解析IP和Port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    // 创建client端和socket

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 进行连接
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    while (1)
    {
        cout << "========================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.quit" << endl;
        cout << "========================" << endl;
        cout << "choice: ";
        int choice;
        cin >> choice;
        cin.get(); // 读掉缓冲区的回车
        switch (choice)
        {
        case 1: // login
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid: ";
            cin >> id;
            cin.get();
            cout << "user password: ";
            cin.getline(pwd, 50);
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << " send login msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv login msg error: " << request << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"].get<int>() != 0)
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        // 登录成功
                        cout << "================login success!=================" << endl;

                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录当前用户的好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str); // 反序列化
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.emplace_back(user);
                            }
                        }
                        // 读取群组信息
                        if (responsejs.contains("groups"))
                        {
                            vector<string> vec = responsejs["groups"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                Group group;
                                group.setId(js["groupid"].get<int>());
                                group.setName(js["groupname"]);
                                group.setDesc(js["groupdesc"]);
                                vector<string> vec2 = js["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }
                        // 显示用户基本信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息，个人消息或者群组消息

                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                cout << js["time"] << " [" << js["id"] << " ]" << js["name"]
                                     << ": " << js["msg"] << endl;
                            }
                        }

                        // 登录成功，启动线程接受线程负责接受数据
                        std::thread readTask(readTaskHandler, clientfd);
                        readTask.detach();
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2: // register业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username: ";
            cin.getline(name, 50);
            cout << "password: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << " send reg msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv reg response error: " << request << endl;
                }
                else
                {
                    json reponsejs = json::parse(buffer);
                    if (reponsejs["errno"].get<int>() != 0)
                    {
                        cerr << name << " is already exist,register error!" << endl;
                    }
                    else
                    {
                        cerr << name << " register success! userid is " << reponsejs["id"]
                             << ", do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3:
            close(clientfd);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;
}

void showCurrentUserData()
{
    cout << "==========================login user==========================" << endl;
    cout << "current login user =>id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << endl;
    cout << "----------------------------friend list-------------------------" << endl;
    if (!g_currentUserFriendList.empty())
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    cout << "----------------------------group list-------------------------" << endl;
    if (!g_currentUserGroupList.empty())
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " "
                     << user.getRole() << endl;
            }
        }

    cout << "=============================================================" << endl;
}

void readTaskHandler(int clientfd)
{
    while (1)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }
        // 接受服务器转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        // 一对一聊天
        if (js["msgid"].get<int>() == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << ": " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (js["msgid"].get<int>() == GROUP_CHAT_MSG)
        {
            int groupid = js["groupid"].get<int>();

            cout << js["time"].get<string>() << " groupname: " << js["groupname"].get<string>() << " [" << js["groupid"] << "] ----->" << " [" << js["id"] << "] "
                 << js["name"].get<string>() << ": " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}
void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friend:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "添加群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销登录，格式loginout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"quit", loginout}};

void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    for (;;)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (idx == -1) // 如果没有：,表示是help或loginout
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        // 调用命令对应的回调函数，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int, string)
{
    cout << "show command list>>>" << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << ": " << p.second << endl;
    }
    cout << endl;
}

void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addfriend msg error ->" << buffer << endl;
    }
}

void chat(int clientfd, string str)
{
    int idx = str.find(":");
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "chat addfriend msg error ->" << buffer << endl;
    }
}

void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "chat addfriend msg error ->" << buffer << endl;
    }
    else
    {
        char buffer[1024] = {0};
        len = recv(clientfd, buffer, 1024, 0);
        if (len == -1)
        {
            cerr << "recv reg response error: " << buffer << endl;
        }
        else
        {
            json reponsejs = json::parse(buffer);
            if (reponsejs["errno"].get<int>() != 0)
            {
                cerr << groupname << " is already exist, create error!" << endl;
            }
            else
            {
                cerr << groupname << " register success! userid is " << reponsejs["groupid"]
                     << ", do not forget it!" << endl;
            }
        }
    }
}

// 加群
void addgroup(int clientfd, string str)
{
    //"addgroup", "加入群组，格式addgroup:groupid"
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup msg error ->" << buffer << endl;
    }
}
void groupchat(int clientfd, string str)
{
    // groupchat:groupid:message
    int idx = str.find(":");
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    // 找到群名
    for (auto &group : g_currentUserGroupList)
    {
        if (group.getId() == groupid)
        {
            js["groupname"] = group.getName();
            break;
        }
    }

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "groupchat send msg error ->" << buffer << endl;
    }
}

void loginout(int clientfd, string str)
{
}
string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();

    // 转换为 time_t 类型（秒）
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // 将时间转换为本地时间
    std::tm *local_time = std::localtime(&now_time);
    // 定义一个缓冲区来存储格式化后的时间字符串
    char buffer[80]; // 足够存放 "YYYY-MM-DD HH:MM:SS" 字符串
    // 格式化时间为 YYYY-MM-DD HH:MM:SS
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_time);
    return std::string(buffer);
}