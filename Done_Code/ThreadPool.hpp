#pragma once

#include <iostream>
#include <queue>
#include <pthread.h>
#include "Log.hpp"
#include "Task.hpp"

#define NUM 6

class ThreadPool{
    private:
        int num;
        bool stop;
        std::queue<Task> task_queue;
        pthread_mutex_t lock;
        pthread_cond_t cond;

        ThreadPool(int _num = NUM):num(_num),stop(false)
        {
            pthread_mutex_init(&lock, nullptr);
            pthread_cond_init(&cond, nullptr);
        }
        ThreadPool(const ThreadPool &){}

        static ThreadPool *single_instance;
    public:
        static ThreadPool* getinstance()
        {
            static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
            if(single_instance == nullptr){
                pthread_mutex_lock(&_mutex);
                if(single_instance == nullptr){
                    single_instance = new ThreadPool();
                    single_instance->InitThreadPool();
                }
                pthread_mutex_unlock(&_mutex);
            }
            return single_instance;
        }

        bool IsStop()
        {
            return stop;
        }
        bool TaskQueueIsEmpty()
        {
            return task_queue.size() == 0 ? true : false;
        }
        void Lock()
        {
            pthread_mutex_lock(&lock);
        }
        void Unlock()
        {
            pthread_mutex_unlock(&lock);
        }
        void ThreadWait()
        {
            pthread_cond_wait(&cond, &lock);
        }
        void ThreadWakeup()
        {
            pthread_cond_signal(&cond);
        }
        static void *ThreadRoutine(void *args)
        {
            ThreadPool *tp = (ThreadPool*)args;

            while(true){
                Task t;
                tp->Lock();
                while(tp->TaskQueueIsEmpty()){
                    tp->ThreadWait(); //当我醒来的时候，一定是占有互斥锁的！
                }
                tp->PopTask(t);
                tp->Unlock();
                t.ProcessOn();
            }
        }
        bool InitThreadPool()
        {
            for(int i = 0; i < num; i++){
                pthread_t tid;
                if( pthread_create(&tid, nullptr, ThreadRoutine, this) != 0){
                    LOG(FATAL, "create thread pool error!");
                    return false;
                }
            }
            LOG(INFO, "create thread pool success!");
            return true;
        }
        void PushTask(const Task &task) //in
        {
            Lock();
            task_queue.push(task);
            Unlock();
            ThreadWakeup();
        }
        void PopTask(Task &task) //out
        {
            task = task_queue.front();
            task_queue.pop();
        }
        ~ThreadPool()
        {
            pthread_mutex_destroy(&lock);
            pthread_cond_destroy(&cond);
        }
};

ThreadPool* ThreadPool::single_instance = nullptr;




