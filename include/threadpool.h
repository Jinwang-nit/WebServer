#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<pthread.h>
#include<list>
#include"lock.h"
#include<cstdio>

// 线程池类，T是任务类
template<typename T> 
class threadpool
{
public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    bool append(T* request);
    ~threadpool();

private:
    static void* worker(void *arg);
    void run();
private:
    int m_thread_number; // 线程数量
    pthread_t *m_threads; // 线程池数组

    int m_max_requests; // 请求队列的上限
    std::list<T*> m_workqueue; // 请求队列

    locker m_queuelocker; // 互斥锁
    sem m_queuestate; // 判断任务是否需要处理

    bool m_stop; // 是否结束线程
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL)
{
    if (thread_number <= 0 || max_requests <= 0)
    {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number]; // pthread_t是整型数，理解为线程ID
    if (!m_threads) {throw std::exception(); }

    // 创建m_thread_number个线程，并设置为线程脱离
    for (int i = 0; i < m_thread_number; i++)
    {
        printf("create the %dth thread\n", i);

        // pthread_create*()就是创建一个线程，参数：线程ID，线程属性传null代表默认，这个线程要做什么，worker函数的参数
        // pthread_create也可以理解为一个线程启动器，他会自己去执行worker函数，不需要显示调用
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete [] m_threads;
            throw std::exception();
        }

        // pthread_detach将线程设置成分离态，分离态线程结束后自动释放资源
        if (pthread_detach(m_threads[i]) != 0)
        {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }

    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestate.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void *arg)
{
    threadpool *pool  = (threadpool*)arg;
    pool->run();

}

template<typename T>
void threadpool<T>::run()
{
    while (!m_stop)
    {
        m_queuestate.wait(); // PV操作里面的P，任务队列里面有没有东西
        m_queuelocker.lock(); // 任务队列锁
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }

        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();

        if (!request) continue;

        request->process();
    }
}
#endif