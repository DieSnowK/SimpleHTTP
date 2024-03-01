#pragma once
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "
#define WEB_ROOT  "webRoot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.0"
#define OK 200
#define NOT_FOUND 404

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
    std::string path;
    std::string arg;

    bool cgi;

    HttpRequest()
        : content_length(0)
        , cgi(false)
    {}
};

struct HttpResponse
{
    std::string status_line;
    std::vector<std::string> response_header;
    std::string blank;
    std::string response_body;
    int status_code;

    HttpResponse()
        : status_code(OK)
    {}
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
        RecvRequestLine();
        RecvRequestHeader();
        ParseRequest();
    }

    void BuildResponse()
    {
        if(_httpRequest.method != "GET" && _httpRequest.method != "POST")
        {
            // 非法请求
            _httpResponse.status_code = NOT_FOUND; // TODO
            LOG(WARNING, "Method is not right");
            goto END;
        }

        if (_httpRequest.method == "GET")
        {
            // GET可能带参数，也可能不带参数，要区分出来
            if (_httpRequest.uri.find('?') != std::string::npos)
            {
                Util::CutString(_httpRequest.uri, _httpRequest.path, _httpRequest.arg, "?");
                _httpRequest.cgi = true;
            }
            else
            {
                _httpRequest.path = _httpRequest.uri;
            }
        }
        else if(_httpRequest.method == "POST")
        {
            _httpRequest.cgi = true;
        }
        else
        {
            // Do Nothing
        }

        _httpRequest.path.insert(0, WEB_ROOT); // 从根目录开始

        LOG(DEBUG, _httpRequest.path);
        LOG(DEBUG, _httpRequest.arg);

        // 如果访问的是根目录，则默认访问主页
        if(_httpRequest.path[_httpRequest.path.size() - 1] == '/')
        {
            _httpRequest.path += HOME_PAGE;
        }

        LOG(DEBUG, _httpRequest.path);

        // 确认访问资源是否存在
        struct stat st;
        if(stat(_httpRequest.path.c_str(), &st) == 0) // TODO  待整理stat()
        {
            // 访问的是否是一个具体资源？
            if(S_ISDIR(st.st_mode))
            {
                // 请求的是一个目录，需要特殊处理 -- 改为访问该目录下主页
                // 虽然是目录，但是绝对不会以/结尾
                _httpRequest.path += "/";
                _httpRequest.path += HOME_PAGE;
            }

            // 是否是一个可程序程序？
            if (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)
            {
                _httpRequest.cgi = true; // TODO CGI标志位感觉有多余
            }
        }
        else
        {
            // 资源不存在
            LOG(WARNING, _httpRequest.path + "Not Found");
            _httpResponse.status_code = NOT_FOUND;
            goto END;
        }

        if(_httpRequest.cgi)
        {
            // ProcessCgi();
        }
        else
        {
            // 1.至此，目标网页一定是存在的
            // 2.返回并不是单单返回网页，而是要构建HTTP响应 // TODO
            ProcessNonCgi(); // 返回静态网页
        }

    END:
        return;
    }

    void SendResponse()
    {}

private:
    void ParseRequest()
    {
        ParseRequestLine();
        ParseRequestHeader();
        RecvRequestBody();
    }

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

        // TODO 可整理
        // 可能不是所有人都严格遵守标准，所以将method统一转化为大写
        std::transform(_httpRequest.method.begin(), _httpRequest.method.end(), _httpRequest.method.begin(), ::toupper);

        LOG(DEBUG, _httpRequest.method);
        LOG(DEBUG, _httpRequest.uri);
        LOG(DEBUG, _httpRequest.version);
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
                else
                {
                    // TODO
                    break;
                }
            }

            LOG(DEBUG, body);
        }
    }

    int ProcessNonCgi()
    {
        _httpResponse.status_line = HTTP_VERSION;
        _httpResponse.status_line += " ";
        _httpResponse.status_line += std::to_string(_httpResponse.status_code);
        _httpResponse.status_line += " ";


        return 0;
    }

    std::string Code2Desc(int code)
    {
        std::string desc = "";
        switch(code)
        {
        case 200:
            desc = "OK";
            break;
        case 400:
            desc = "404";
            break;
        default:
            break;
        }

        return desc;
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
        ep->BuildResponse();
        ep->SendResponse();
        delete ep;
        
        LOG(INFO, "Hander Request Begin");
        return nullptr;
    }
};
