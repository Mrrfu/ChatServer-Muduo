#include "friendmodel.hpp"
#include "db.hpp"

void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend values(%d,%d)", userid, friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

std::vector<User> FriendModel::query(int userid)
{
    /*
    SELECT u.id, u.name,u.state
    FROM User u
    JOIN Friend f ON f.friend = u.id
    WHERE f.userid = userid;

    */
    char sql[1024] = {0};
    sprintf(sql, "SELECT u.id, u.name, u.state FROM User u JOIN Friend f ON f.friendid = u.id WHERE f.userid = %d;", userid);
    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.emplace_back(user);
            }
            mysql_free_result(res); // 注意释放资源
        }
    }
    return vec;
}
