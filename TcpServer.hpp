#pragma once
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "Log.hpp"

static const uint16_t PORT = 8090;
static const int BACKLOG = 128;

// 鍗曚緥 -- 楗挎眽妯″紡
class TcpServer
{
public:
    static TcpServer* GetInstance(uint16_t port = PORT)
    {
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        if(svr == nullptr) // 鍙岄噸鍒ょ┖鎸囬拡锛岄檷浣庨攣鍐茬獊鐨勬鐜囷紝鎻愰珮鎬ц兘
        {
            // 娉ㄦ剰绾跨▼瀹夊叏
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
        _listenSock = socket(AF_INET, SOCK_STREAM, 0);
        if(_listenSock < 0)
        {
            LOG(FATAL, "Socket Error");
            exit(1);
        }

        // 璁剧疆绔彛澶嶇敤
        int opt = 1;
        setsockopt(_listenSock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        LOG(INFO, "Create Listen Socket ... Success");
    }

    void Bind()
    {
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(_port);

        if(bind(_listenSock, (struct sockaddr*)&local, sizeof(local)) < 0)
        {
            LOG(FATAL, "Bind Error");
            exit(2);
        }
        LOG(INFO, "Bind Socket ... Success");
    }

    void Listen()
    {
        if(listen(_listenSock, BACKLOG) < 0)
        {
            LOG(FATAL, "Listen Error");
            exit(3);
        }
        LOG(INFO, "Listen Socket ... Success");
    }

    int Sock()
    {
        return _listenSock;
    }

    ~TcpServer()
    {
        if(_listenSock >= 0)
        {
            close(_listenSock);
        }
    }
private:
    TcpServer(uint16_t port)
        : _port(port)
        , _listenSock(-1)
    {}

    TcpServer(const TcpServer&) = delete;
private:
    uint16_t _port;
    int _listenSock;
    static TcpServer* svr;
};

TcpServer* TcpServer::svr = nullptr;
