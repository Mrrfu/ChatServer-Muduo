#pragma once
#include <vector>
#include "user.hpp"
// 维护好友信息的操作接口
class FriendModel
{
public:
    // 添加好友
    void insert(int userid, int friendid);
    // 删除好友
    // void remove(int userid, int friendid);

    // 返回用户好友列表 friendid && name
    std::vector<User> query(int userid);
};