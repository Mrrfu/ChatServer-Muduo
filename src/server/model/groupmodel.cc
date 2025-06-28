#include "groupmodel.hpp"
#include "db.hpp"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname,groupdesc) values('%s','%s')",
            group.getName().c_str(), group.getDesc().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, std::string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser(groupid,userid,grouprole) values(%d,%d,'%s')",
            groupid, userid, role.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 根据用户id查找所加入的群组
std::vector<Group> GroupModel::queryGroups(int userid)
{
    std::vector<Group> groupVec;
    char sql[1024] = {0};
    sprintf(sql, "SELECT g.id, g.groupname, g.groupdesc FROM GroupUser u JOIN AllGroup g ON u.groupid = g.id WHERE u.userid = %d", userid);
    vector<Group> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.emplace_back(group);
            }
            mysql_free_result(res); // 注意释放资源
        }
    }
    // 查询群组的用户信息
    for (Group &group : groupVec)
    {
        sprintf(sql, "SELECT a.id, a.name, a.state, b.grouprole from User a \
            inner join GroupUser b ON b.userid = a.id WHERE b.groupid = %d",
                group.getId());
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().emplace_back(user);
            }
            mysql_free_result(res); // 注意释放资源
        }
    }
    return groupVec;
}

std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT userid FROM GroupUser WHERE groupid = %d AND userid != %d", groupid, userid);
    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.emplace_back(atoi(row[0]));
            }
            mysql_free_result(res); // 注意释放资源
        }
    }
    return idVec;
}