#pragma once

#include <vector>
#include <string>
#include <unistd.h>
class HttpRequest
{
    std::string requestLine;
    std::vector<std::string> request_header;
    std::string blank;
};

class HttpResponse
{

};

// 负责两端业务处理，主要包括以下功能
// 读取请求，分析请求，构建相应 IO通信
class EndPoint
{
public:
    EndPoint(int sock)
        : _sock(sock)
    {}

    ~EndPoint()
    {
        close(_sock);
    }

    void RecvRequest()
    {}

    void ParseRequest()
    {}

    void BuildResponse()
    {}

    void SendResponse()
    {}


private:
    int _sock;
    HttpRequest httpRequest;
    HttpResponse httpResponse;
};

struct Entrance
{
    // 这里设置static是为了给thread传start_routine时，参数可以对应
    static void* HandlerRequest(void* arg)
    {
        EndPoint *ep = new EndPoint(sock);
        ep->RecvRequest();
        ep->ParseRequest();
        ep->BuildResponse();
        ep->SendResponse();
        delete ep;

        return nullptr;
    }
};