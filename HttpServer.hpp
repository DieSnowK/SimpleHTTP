#pragma once
#include <signal.h>
#include "TcpServer.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"

class HttpServer
{
public:
    HttpServer(int port = PORT)
        : _port(port)
        , _stop(false)
    {}

    void Init()
    {
        // 信号SIGPIPE需要进行忽略，如果不忽略，在写入的时候，可能直接崩溃server
        signal(SIGPIPE, SIG_IGN);
    }

    void Loop(int threadNum = THREAD_POOL_NUM)
    {
        TcpServer *tsvr = TcpServer::GetInstance(_port);
        
        LOG(INFO, "Loop Begin");
        while(!_stop)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);

            int sock = accept(tsvr->Sock(), (struct sockaddr *)&peer, &len);
            if(sock < 0)
            {
                continue;
            }
            LOG(INFO, "Get a new link");

            Task task(sock);
            ThreadPool::GetInstance(threadNum)->Push(task);
        }
    }

private:
    uint16_t _port;
    bool _stop;
};
