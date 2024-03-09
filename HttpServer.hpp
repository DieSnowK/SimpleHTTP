#pragma once
#include <signal.h>
#include "TcpServer.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"

static const uint16_t PORT = 8090;

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
        signal(SIGPIPE, SIG_IGN); // TODO 待整理 #36
    }

    void Loop()
    {
        TcpServer *tsvr = TcpServer::GetInstance(_port);
        int listenSock = tsvr->Sock();

        LOG(INFO, "Loop Begin");
        while(!_stop)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);

            int sock = accept(listenSock, (struct sockaddr *)&peer, &len);
            if(sock < 0)
            {
                continue;
            }
            LOG(INFO, "Get a new link");

            // v2.0
            Task task(sock);
            _threadPool.Push(task);
        }
    }

private:
    uint16_t _port;
    bool _stop;
    ThreadPool _threadPool;
};
