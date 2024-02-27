#pragma once

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "Log.hpp"

static const int BACKLOG = 128;

class TcpServer
{
public:
    static TcpServer* GetInstance(uint16_t port)
    {
        // zhu yi xian cheng an quan
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        if(svr == nullptr) // WHY?
        {
            pthread_mutex_lock(&lock);
            if(svr == nullptr)
            {
                svr = new TcpServer(port);
                svr->Init();
            }
            pthread_mutex_unlock(&lock);
        }

        return svr;
    }

    void Init()
    {
        Socket();
        Bind();
        Listen();
        LOG(INFO, "TcpServer Init ... Success");
    }

    void Socket()
    {
        _listensock = socket(AF_INET, SOCK_STREAM, 0);
        if(_listensock < 0)
        {
            LOG(FATAL, "Socket Error");
            exit(1);
        }

        // TODO Write
        int opt = 1;
        setsockopt(_listensock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    }

    void Bind()
    {
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(_port);

        if(bind(_listensock, (struct sockaddr*)&local, sizeof(local)) < 0)
        {
            LOG(FATAL, "Bind Error");
            exit(2);
        }
    }

    void Listen()
    {
        if(listen(_listensock, BACKLOG) < 0)
        {
            LOG(FATAL, "Listen Error");
            exit(3);
        }
    }

    ~TcpServer()
    {
        if(_listensock >= 0)
        {
            close(_listensock);
        }
    }
private:
    TcpServer(uint16_t port)
      :_port(port)
      ,_listensock(-1)
    {}

    TcpServer(const TcpServer&) = delete;
private:
    uint16_t _port;
    int _listensock;
    static TcpServer* svr;
};

TcpServer* TcpServer::svr = nullptr;
