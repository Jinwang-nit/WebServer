#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include<sys/epoll.h>
#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<stdarg.h>
#include<errno.h>
#include "lock.h"
#include<sys/uio.h>
#include<string.h>
#include<vector>
#include<string>
#include "mysql_conn.h"
class http_conn
{
public:

    static int m_epollfd;
    static int m_user_count;
    static const int FILENAME_LEN = 200; 
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;

#pragma region // ***************状态机****************//
   // HTTP请求方法，这里只支持GET
   enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    
   /*
       解析客户端请求时，主状态机的状态
       CHECK_STATE_REQUESTLINE:当前正在分析请求行
       CHECK_STATE_HEADER:当前正在分析头部字段
       CHECK_STATE_CONTENT:当前正在解析请求体
   */
   enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
   
   /*
       服务器处理HTTP请求的可能结果，报文解析的结果
       NO_REQUEST          :   请求不完整，需要继续读取客户数据
       GET_REQUEST         :   表示获得了一个完成的客户请求
       BAD_REQUEST         :   表示客户请求语法错误
       NO_RESOURCE         :   表示服务器没有资源
       FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
       FILE_REQUEST        :   文件请求,获取文件成功
       INTERNAL_ERROR      :   表示服务器内部错误
       CLOSED_CONNECTION   :   表示客户端已经关闭连接了
   */
   enum HTTP_CODE 
   { NO_REQUEST, // 请求不完整，需要继续读取客户数据
    GET_REQUEST, // 表示获得了一个完成的客户请求
    BAD_REQUEST, // 表示客户请求语法错误
    NO_RESOURCE, // 表示服务器没有资源
    FORBIDDEN_REQUEST, // 表示客户对资源没有足够的访问权限
    FILE_REQUEST, // 文件请求,获取文件成功
    INTERNAL_ERROR, // 表示服务器内部错误
    CLOSED_CONNECTION, // 表示客户端已经关闭连接了
    POST_SUCCESS, // POST成功
    POST_FAIL, // POST失败
};
   
   // 从状态机的三种可能状态，即行的读取状态，分别表示
   // 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整
   enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

// ***************状态机****************//
    http_conn(){}
    ~http_conn(){}

    void process();
    void init(int sockfd, const sockaddr_in &addr);
    void close_conn();
    bool read(); // 非阻塞的读
    bool write(); // 非阻塞的写
    HTTP_CODE process_read(); // 解析请求
    bool process_write(HTTP_CODE ret); // 解析请求
    HTTP_CODE parse_request_line(char *text); // 解析请求首行
    HTTP_CODE parse_header(char *text); // 解析请求头
    HTTP_CODE parse_content(char *text); // 解析请求体

    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_content_type();
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

    char m_write_buf[ WRITE_BUFFER_SIZE ];  // 写缓冲区
    int m_write_idx;                        // 写缓冲区中待发送的字节数
    char* m_file_address;                   // 客户请求的目标文件被mmap到内存中的起始位置
    struct stat m_file_stat;                // 目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
    struct iovec m_iv[2];                   // 我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量。
    int m_iv_count;
    int bytes_to_send;
    int bytes_have_send;

    LINE_STATUS parse_line();
    HTTP_CODE do_request();

    char *get_line(){return m_read_buffer + m_start_line;}
    void parse_post_content(char* text);
#pragma endregion

#pragma region // ***************数据库****************//
    enum DB_CODE
    {
        OPT_FAIL,
        OPT_SUCCESS,
    };



    void init_mysql();  // 初始化数据库连接池
    DB_CODE do_check_db(const char* username, const char* password); // 检查数据库信息是否存在
    DB_CODE do_insert_db(const char* username, const char* password, const char* message); // 插入信息
    DB_CODE do_log_access();
#pragma endregion

private:
    int m_socketfd;  // http连接的socket
    sockaddr_in m_address; // socket地址
    char m_read_buffer[READ_BUFFER_SIZE]; // 读缓存区
    int m_read_idx; // 下次从哪开始
    int m_checked_idx; // 当前分析的字符在读缓冲区的位置
    int m_start_line; // 当前行的起始位置

    // get
    char* m_url;
    char* m_version;
    METHOD m_method;
    char* m_host;
    char m_real_file[FILENAME_LEN];
    bool m_linger; // 是否保持连接
    int m_content_length;

    // post
    const char* m_username;
    const char* m_password;
    const char* m_message;

    CHECK_STATE m_check_state;

    void init(); // 初始化其余信息

    
};

#endif