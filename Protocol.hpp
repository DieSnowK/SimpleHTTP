#pragma once
#include <vector>
#include <string>
#include <unistd.h>
#include "Util.hpp"
#include "Log.hpp"
class HttpRequest
{
public:
    std::string request_line;
    std::vector<std::string> request_header;
    std::string blank;
    std::string request_body;

    // Parse Ret
    std::string method;
    std::string uri;
    std::string version;
};

class HttpResponse
{
public:
    std::string status_line;
    std::vector<std::string> response_header;
    std::string blank;
    std::string response_body;
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
    {
        RecvRequestHeader();
        RecvRequestHeader();
    }

    void BuildResponse()
    {}

    void SendResponse()
    {}

private:
    void RecvRequestLine()
    {
        Util::ReadLine(_sock, _httpRequest.request_line);
    }

    void RecvRequestHeader()
    {
        std::string line;
        while(true)
        {
            line.clear();
            if(Util::ReadLine(_sock, line) <= 0)
            {
                break;
            }

            if(line == '\n')
            {
                _httpResponse.response_header = line;
                break;
            }

            line.resize(line.size() - 1); // No Blank
            _httpRequest.request_header.push_back(line);
            

        }
    }

    void ParseRequest()
    {}
private:
    int _sock;
    HttpRequest _httpRequest;
    HttpResponse _httpResponse;
};

struct Entrance
{
    // 这里设置static是为了给thread传start_routine时，参数可以对应
    static void* HandlerRequest(void* arg)
    {
        LOG(INFO, "Hander Request Begin");
        EndPoint *ep = new EndPoint(sock); // TODO
        ep->RecvRequest();
        ep->ParseRequest();
        ep->BuildResponse();
        ep->SendResponse();
        delete ep;
        
        LOG(INFO, "Hander Request Begin");
        return nullptr;
    }
};
