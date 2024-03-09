#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>
#include "Task.hpp"

class ThreadPool
{
public:
    static void *ThreadRoutine(void *args)
    {

    }

    void Init()
    {

    }

    void Push(const Task& task) // in
    {

    }

    void Pop() // out
    {

    }

    void ThreadWait()
    {

    }

    void ThreadWakeUp()
    {

    }

    bool IsStop()
    {
        
    }

    ThreadPool()
    {}

    ~ThreadPool()
    {}
private:
    int _num;
    bool _stop;
    std::queue<Task> _taskQueue;
    pthread_mutex_t _mtx;
    pthread_cond_t _cond;
};