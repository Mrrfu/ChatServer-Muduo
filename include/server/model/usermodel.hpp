#pragma once
#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    // 插入新用户
    bool insert(User &user);
    // 根据ID查询用户
    User query(int id);
    // 更新用户的状态信息
    bool updateState(User &user);
    // 重置用户的状态信息

    void reset();
};
