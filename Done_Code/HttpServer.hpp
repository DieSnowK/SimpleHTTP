#pragma once

#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "Log.hpp"
#include "TcpServer.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"

#define PORT 8081

class HttpServer{
    private:
        int port;
        bool stop;
    public:
        HttpServer(int _port = PORT): port(_port),stop(false)
        {}
        void InitServer()
        {
            //信号SIGPIPE需要进行忽略，如果不忽略，在写入时候，可能直接崩溃server
            signal(SIGPIPE, SIG_IGN); 
        }
        void Loop()
        {
            TcpServer *tsvr = TcpServer::getinstance(port);
            LOG(INFO, "Loop begin");
            while(!stop){
                struct sockaddr_in peer;
                socklen_t len = sizeof(peer);
                int sock = accept(tsvr->Sock(), (struct sockaddr*)&peer, &len);
                if(sock < 0){
                    continue;
                }
                LOG(INFO, "Get a new link");
                Task task(sock);
                ThreadPool::getinstance()->PushTask(task);
            }
        }
        ~HttpServer()
        {}
};
