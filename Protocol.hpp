#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <sstream>
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "

struct HttpRequest
{
    std::string request_line;
    std::vector<std::string> request_header;
    std::string blank;
    std::string request_body;

    // 解析结果
    std::string method;
    std::string uri;
    std::string version;
    std::unordered_map<std::string, std::string> headerMap;
    size_t content_length;
};

struct HttpResponse
{
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
        _httpRequest.request_line.resize(_httpRequest.request_line.size() - 1); // 去掉结尾换行符
        LOG(DEBUG, _httpRequest.request_line);
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

            if(line == "\n")
            {
                _httpRequest.blank = line;
                break;
            }

            line.resize(line.size() - 1); // 去掉结尾换行符
            _httpRequest.request_header.push_back(line);
            
            LOG(DEBUG, line); 
        }
    }

    void ParseRequestLine()
    {
        std::stringstream ss(_httpRequest.request_line); // TODO 可整理
        ss >> _httpRequest.method >> _httpRequest.uri >> _httpRequest.version;
    }

    void ParseRequestHeader()
    {
        std::string key;
        std::string value;
        for(auto& str : _httpRequest.request_header)
        {
           if(Util::CutString(str, key, value, SEP))
           {
               _httpRequest.headerMap[key] = value;
           }
        }
    }

    bool IsRecvRequestBody()
    {
        std::string& method = _httpRequest.method;
        if(method == "POST")
        {
            _httpRequest.content_length = atoi(_httpRequest.headerMap["Content-Length"].c_str());
            return true;
        }

        return false;
    }
    
    void RecvRequestBody()
    {
        if(IsRecvRequestBody())
        {
            size_t length = _httpRequest.content_length;
            auto& body = _httpRequest.request_body;
            char ch = 'K';
            
            while(length)
            {
                ssize_t s = recv(_sock, &ch, 1, 0);
                if(s > 0)
                {
                    body.push_back(ch);
                    length--;
                }

                // TODO
            }

            LOG(DEBUG, body);
        }
    }

    void ParseRequest()
    {
        ParseRequestLine();

    }
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
        int sock = *((int*)arg);

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
