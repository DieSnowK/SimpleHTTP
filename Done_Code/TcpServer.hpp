#pragma once

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "Log.hpp"

#define BACKLOG 5

class TcpServer{
    private:
        int port;
        int listen_sock;
        static TcpServer *svr;
    private:
        TcpServer(int _port):port(_port),listen_sock(-1)
        {}
        TcpServer(const TcpServer &s){}
    public:
        static TcpServer *getinstance(int port)
        {
            static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
            if(nullptr == svr){
                pthread_mutex_lock(&lock);
                if(nullptr == svr){
                    svr = new TcpServer(port);
                    svr->InitServer();
                }
                pthread_mutex_unlock(&lock);
            }
            return svr;
        }
        void InitServer()
        {
            Socket();
            Bind();
            Listen();
            LOG(INFO, "tcp_server init ... success");
        }
        void Socket()
        {
            listen_sock = socket(AF_INET, SOCK_STREAM, 0);
            if(listen_sock < 0){
                LOG(FATAL, "socket error!");
                exit(1);
            }
            int opt = 1;
            setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            LOG(INFO, "create socket ... success");
        }
        void Bind()
        {
            struct sockaddr_in local;
            memset(&local, 0, sizeof(local));
            local.sin_family = AF_INET;
            local.sin_port = htons(port);
            local.sin_addr.s_addr = INADDR_ANY; //云服务器不能直接绑定公网IP

            if(bind(listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0){
                LOG(FATAL, "bind error!");
                exit(2);
            }
            LOG(INFO, "bind socket ... success");
        }
        void Listen()
        {
            if(listen(listen_sock, BACKLOG) < 0){
                LOG(FATAL, "listen socket error!");
                exit(3);
            }
            LOG(INFO, "listen socket ... success");
        }
        int Sock()
        {
            return listen_sock;
        }
        ~TcpServer()
        {
            if(listen_sock >= 0) close(listen_sock);
        }
};

TcpServer* TcpServer::svr = nullptr;

