#pragma once
#include "TcpServer.hpp"
#include "Protocol.hpp"

static const uint16_t PORT = 8080;

class HttpServer
{
public:
    HttpServer(int port = PORT)
        : _port(port)
        , _stop(false)
    {}

    void Init()
    {
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

            int *tmpSock = new int(sock);
            pthread_t tid;
            pthread_create(&tid, nullptr, Entrance::HandlerRequest, tmpSock);
            pthread_detach(tid);
        }
    }

private:
    uint16_t _port;
    bool _stop;
};
