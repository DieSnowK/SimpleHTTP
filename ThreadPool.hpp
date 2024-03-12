#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>
#include "Task.hpp"
#include "Log.hpp"

static const int THREAD_POOL_NUM = 10;

// 单例模式
class ThreadPool
{
public:
    static ThreadPool *GetInstance(int num = THREAD_POOL_NUM)
    {
        static pthread_mutex_t sMtx = PTHREAD_MUTEX_INITIALIZER;
        if (_tp == nullptr)
        {
            pthread_mutex_lock(&sMtx);
            if (_tp == nullptr) // 双重判断，以防线程安全问题
            {
                _tp = new ThreadPool(num);
                _tp->Init();
            }
            pthread_mutex_unlock(&sMtx);
        }

        return _tp;
    }

    // static使该成员函数没有this指针，因为线程执行的函数只能有一个void*参数
    static void *ThreadRoutine(void *args)
    {
        ThreadPool *tp = (ThreadPool *)args;

        while(true)
        {
            Task task;
            tp->Lock();
            while(tp->TaskQueueIsEmpty()) // while防止伪唤醒
            {
                tp->ThreadWait();
            }
            tp->Pop(task);
            tp->Unlock(); // 注意，不要在临界资源区内处理任务哦~
            task.ProcessOn();
        }
    }

    bool Init()
    {
        for (int i = 0; i < _num; i++)
        {
            pthread_t tid;
            if (pthread_create(&tid, nullptr, ThreadRoutine, this) != 0)
            {
                LOG(FATAL, "Create ThreadPool Error");
                return false;
            }
        }
        LOG(INFO, "Create ThreadPool Success");
        
        return true;
    }

    void Push(const Task& task) // in
    {
        Lock();
        _taskQueue.push(task); // 任务队列为临界资源，操作要加锁
        Unlock();

        ThreadWakeUp();
    }

    void Pop(Task& task) // out
    {
        task = _taskQueue.front();
        _taskQueue.pop();
    }

    void ThreadWait()
    {
        pthread_cond_wait(&_cond, &_mtx);
    }

    void ThreadWakeUp()
    {
        pthread_cond_signal(&_cond);
    }

    bool TaskQueueIsEmpty()
    {
        return !_taskQueue.size();
    }

    void Lock()
    {
        pthread_mutex_lock(&_mtx);
    }

    void Unlock()
    {
        pthread_mutex_unlock(&_mtx);
    }

    bool IsStop()
    {
        return _stop;
    }

    ~ThreadPool()
    {
        pthread_mutex_destroy(&_mtx);
        pthread_cond_destroy(&_cond);
    }
private:
    ThreadPool(int num = THREAD_POOL_NUM)
        : _num(num), _stop(false)
    {
        pthread_mutex_init(&_mtx, nullptr);
        pthread_cond_init(&_cond, nullptr);
    }

    ThreadPool(const ThreadPool &) = delete;
private:
    int _num;
    bool _stop;
    std::queue<Task> _taskQueue;
    pthread_mutex_t _mtx;
    pthread_cond_t _cond;
    static ThreadPool *_tp;
};

ThreadPool* ThreadPool::_tp = nullptr;