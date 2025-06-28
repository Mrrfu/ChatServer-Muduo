#pragma once
#include <string>
#include <iostream>
#include <vector>
#include "user.hpp"

class Group
{
public:
    Group(int id = -1, std::string name="", std::string desc="") : id(id), name(name), desc(desc) {}

    // getter and setter
    void setId(const int &id) { this->id = id; }
    void setName(const std::string &name) { this->name = name; }
    void setDesc(const std::string &desc) { this->desc = desc; }

    int getId() { return this->id; }
    std::string getName() { return this->name; }
    std::string getDesc() { return this->desc; }

    // 获取组成员的信息
    std::vector<GroupUser> &getUsers() { return this->users; }

private:
    int id;
    std::string name;
    std::string desc;
    std::vector<GroupUser> users;
};