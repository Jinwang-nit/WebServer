#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include "lock.h"
#include "threadpool.h"
#include<signal.h>
#include "http_conn.h"

#define MAX_FD 65535 // 最大客户端数量
#define MAX_EVENT_NUMBER 10000 // 监听的最大事件数量

// 添加信号捕捉
void addsig(int sig, void(handler)(int))
{
    struct sigaction sa; // 描述信号处理方式的结构体，类似PCB样的东西
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask); // 阻塞其它信号
    sigaction(sig, &sa, NULL);
}

// 添加文件描述符
extern void addfd(int epollfd, int fd, bool one_shot);
// 删除文件描述符
extern void removefd(int epollfd, int fd);
// 修改文件描述符
extern void modifyfd(int epollfd, int fd, int ev);

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        printf("请按如下格式运行： %s port_number\n", basename(argv[0]));
        exit(-1);
    }

    // 获取端口号
    int port = atoi(argv[1]);

    // 对SIGPIE信号处理，忽略SIGPIE信号， 当服务器向某个断开连接的客户端写数据的时候会收到内核传来的SIGPIPE信号这个信号默认会把当前进程停掉，但是我们不希望服务端被停掉
    // 所以只需要忽视这个信号就行了
    addsig(SIGPIPE, SIG_IGN);

    // 创建线程池，池子里面的八个线程会一直去访问任务队列并执行任务
    threadpool<http_conn> *pool = NULL;
    try
    {
        pool = new threadpool<http_conn>;
    }
    catch(...)
    {
        exit(-1);
    }
    

    // 创建一个数组用于保存所有的客户端信息
    http_conn *users = new http_conn[MAX_FD];

    // 创建监听套接字，理解为通信接口，得先有接口然后再拿这个接口去绑定ip，端口这些
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // 端口复用：TCP连接断开的时候会有2MSL等待时间, 如果这个时间段内服务器重启了，但是因为时间没过端口没用释放
    // 导致连接失败
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 绑定
    struct sockaddr_in address; // 用来存ip地址和端口
    address.sin_family = AF_INET; // 用ipv4
    address.sin_addr.s_addr = INADDR_ANY; // 任何ip都可以访问
    address.sin_port = htons(port);
    bind(listenfd, (struct sockaddr*)&address, sizeof(address));

    // 监听端口
    listen(listenfd, 5);

    // 创建epoll对象, 事件数组， 添加
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);

    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;

    while (true)
    {
        // 取事件，-1代表永久阻塞，num代表有多少个socket产生了事件
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (num < 0 && (errno != EINTR))
        {
            printf("epoll fall\n");
            break;
        }

        for (int i = 0; i < num; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                // 有客户端链接
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlen);

                if (http_conn::m_user_count >= MAX_FD)
                {
                    close(connfd); // 关闭连接
                    continue;
                }

                // if (connfd == -1)
                // {
                //     printf("出现accept失败\n");
                //     exit(0);
                // }

                // 将新客户端初始化放到数组中
                users[connfd].init(connfd, client_address);

            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) // 三个宏代表三种异常
            {
                printf("ERROR\n");
                // 对方异常断开
                users[sockfd].close_conn();
            }
            else if (events[i].events & EPOLLIN) // EPOLLIN代表缓冲区有数据
            {
                printf("EPOLLIN\n");
                if (users[sockfd].read())
                {
                    pool->append(users + sockfd);
                }
                else
                {
                    users[sockfd].close_conn();
                }
            }
            else if (events[i].events & EPOLLOUT) // EPOLLOUT代表缓冲区有空余空间
            {
                printf("EPOLLOUT\n");
                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;
    return 0;
}