#include "offlinemessagemodel.hpp"
#include "db.hpp"

void OfflineMsgModel::insert(int userid, std::string msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage values(%d,'%s')", userid, msg.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete  from OfflineMessage where userid = %d", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
std::vector<std::string> OfflineMsgModel::query(int userid)
{
    std::vector<std::string> msgList;
    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid =%d", userid);
    MySQL mysql;
    if (mysql.connect())
    {

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                msgList.emplace_back(row[0]);
            }
            mysql_free_result(res);
            return msgList;
        }
    }
    return msgList;
}