#include "db.hpp"
#include <muduo/base/Logging.h>
using namespace muduo;
// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源~MySQL()
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
bool MySQL::connect()
{
    if (!_conn)
    {
        LOG_INFO << "inti mysql fail!";
        return false;
    }
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        // C和C++代码默认字符是ASCII，如果不设置，从MySql拉取的中文将显示乱码
        mysql_query(_conn, "SET NAMES 'utf8mb4'");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
MYSQL *MySQL::getConnection()
{
    return _conn;
}