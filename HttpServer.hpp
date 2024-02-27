#pragma once

#include "TcpServer.hpp"

static const uint16_t PORT = 8080;

class HttpServer
{
public:
    HttpServer(int port = PORT)
    :_port(port)
    ,_stop(false)
    {}


private:
    uint16_t _port;
    bool _stop;
};
