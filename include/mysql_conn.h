#ifndef MYSQL_CONN_H
#define MYSQL_CONN_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>
#include "mysql_conn.h"

class mysql_conn
{
public:
    mysql_conn();
    ~mysql_conn();

    // 初始化数据库
    bool init(const std::string& host, const std::string& user, const std::string& password, const std::string& dbname, int port = 3306);

    // 断开连接
    void close();
    // 执行查询（SELECT）
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
    // 执行更新（INSERT, UPDATE, DELETE）
    bool update(const std::string& sql);
    // 转义字符串（防止SQL注入）
    std::string escape_string(const std::string& str);

    // 获取最后一次错误信息
    std::string get_last_error();
    // 获取插入的最后一条记录的ID
    int get_insert_id();

private:
    MYSQL* m_conn;
    bool m_connected;
    std::string m_last_error;
};

// 数据库连接池（单例模式）
class mysql_pool 
{
    public:
        static mysql_pool* get_instance();
        
        // 初始化连接池
        bool init(const std::string& host, const std::string& user,
                  const std::string& password, const std::string& dbname,
                  int port = 3306, int max_conn = 10);
        
        // 获取连接
        mysql_conn* get_connection();
        
        // 释放连接
        void release_connection(mysql_conn* conn);
        
        // 销毁连接池
        void destroy();
        
    private:
        mysql_pool();
        ~mysql_pool();
        
        std::vector<mysql_conn*> m_connections;
        std::string m_host;
        std::string m_user;
        std::string m_password;
        std::string m_dbname;
        int m_port;
        int m_max_conn;
        
        static mysql_pool* m_instance;
};

#endif