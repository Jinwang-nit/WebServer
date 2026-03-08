#include "mysql_conn.h"
#include <cstring>
#include <cstdio>

// 单例实例初始化
mysql_pool* mysql_pool::m_instance = nullptr;

// ==================== mysql_conn 实现 ====================

mysql_conn::mysql_conn() : m_conn(nullptr), m_connected(false) {
    m_conn = mysql_init(nullptr);
}

mysql_conn::~mysql_conn() {
    close();
}

bool mysql_conn::init(const std::string& host, const std::string& user,
                      const std::string& password, const std::string& dbname,
                      int port) 
{
    if (!m_conn) {
        m_conn = mysql_init(nullptr);
    }
    
    // 设置字符集为UTF8
    mysql_options(m_conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    
    // 连接数据库
    if (!mysql_real_connect(m_conn, host.c_str(), user.c_str(),
                           password.c_str(), dbname.c_str(), port, nullptr, 0)) {
        m_last_error = mysql_error(m_conn);
        printf("MySQL连接失败: %s\n", m_last_error.c_str());
        return false;
    }
    
    m_connected = true;
    printf("MySQL连接成功\n");
    return true;
}

void mysql_conn::close() {
    if (m_conn) {
        mysql_close(m_conn);
        m_conn = nullptr;
        m_connected = false;
    }
}

std::vector<std::map<std::string, std::string>> mysql_conn::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> result;
    
    if (!m_connected) {
        m_last_error = "未连接到数据库";
        return result;
    }
    
    // 执行查询
    if (mysql_query(m_conn, sql.c_str()) != 0) {
        m_last_error = mysql_error(m_conn);
        printf("查询失败: %s\n", m_last_error.c_str());
        return result;
    }
    
    // 获取结果集
    MYSQL_RES* res = mysql_store_result(m_conn);
    if (!res) {
        m_last_error = mysql_error(m_conn);
        return result;
    }
    
    // 获取字段数量
    int num_fields = mysql_num_fields(res);
    
    // 获取字段名
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    
    // 遍历结果集
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::map<std::string, std::string> row_data;
        for (int i = 0; i < num_fields; i++) {
            std::string field_name = fields[i].name;
            std::string field_value = row[i] ? row[i] : "";
            row_data[field_name] = field_value;
        }
        result.push_back(row_data);
    }
    
    mysql_free_result(res);
    return result;
}

bool mysql_conn::update(const std::string& sql) {
    if (!m_connected) {
        m_last_error = "未连接到数据库";
        return false;
    }
    
    if (mysql_query(m_conn, sql.c_str()) != 0) {
        m_last_error = mysql_error(m_conn);
        printf("更新失败: %s\n", m_last_error.c_str());
        return false;
    }
    
    return true;
}

std::string mysql_conn::get_last_error() {
    return m_last_error;
}

std::string mysql_conn::escape_string(const std::string& str) {
    if (!m_conn) return str;
    
    char* escaped = new char[str.length() * 2 + 1];
    mysql_real_escape_string(m_conn, escaped, str.c_str(), str.length());
    std::string result(escaped);
    delete[] escaped;
    return result;
}

int mysql_conn::get_insert_id() {
    if (!m_connected) return -1;
    return mysql_insert_id(m_conn);
}

// ==================== mysql_pool 实现 ====================

mysql_pool::mysql_pool() : m_port(3306), m_max_conn(10) {
}

mysql_pool::~mysql_pool() {
    destroy();
}

mysql_pool* mysql_pool::get_instance() {
    if (m_instance == nullptr) {
        m_instance = new mysql_pool();
    }
    return m_instance;
}

bool mysql_pool::init(const std::string& host, const std::string& user,
                      const std::string& password, const std::string& dbname,
                      int port, int max_conn) {
    m_host = host;
    m_user = user;
    m_password = password;
    m_dbname = dbname;
    m_port = port;
    m_max_conn = max_conn;
    
    // 创建连接
    for (int i = 0; i < max_conn; i++) {
        mysql_conn* conn = new mysql_conn();
        if (conn->init(host, user, password, dbname, port)) {
            m_connections.push_back(conn);
        } else {
            delete conn;
            printf("创建第%d个连接失败\n", i);
        }
    }
    
    if (m_connections.empty()) {
        printf("连接池初始化失败，没有可用连接\n");
        return false;
    }
    
    printf("MySQL连接池初始化成功，共%d个连接\n", (int)m_connections.size());
    return true;
}

mysql_conn* mysql_pool::get_connection() {
    if (m_connections.empty()) {
        return nullptr;
    }
    
    // 简单实现：取最后一个连接
    mysql_conn* conn = m_connections.back();
    m_connections.pop_back();
    return conn;
}

void mysql_pool::release_connection(mysql_conn* conn) {
    if (conn) {
        m_connections.push_back(conn);
    }
}

void mysql_pool::destroy() {
    for (auto conn : m_connections) {
        delete conn;
    }
    m_connections.clear();
}
