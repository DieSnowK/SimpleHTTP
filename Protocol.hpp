#pragma once
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "
#define LINE_END "\r\n"
#define WEB_ROOT  "wwwroot"
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
    std::string suffix;
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
    int fd;
    int fSize;

    HttpResponse()
        : status_code(OK)
        , fd(-1)
        , blank(LINE_END)
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
        size_t pos = 0;
        if (_httpRequest.method != "GET" && _httpRequest.method != "POST")
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
            _httpRequest.path = _httpRequest.uri;
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

                // 上面更改了path指向，所以重新获取文件状态
                stat(_httpRequest.path.c_str(), &st);
            }

            // 是否是一个可程序程序？
            if (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)
            {
                _httpRequest.cgi = true; // TODO CGI标志位感觉有多余
            }

            _httpResponse.fSize = st.st_size;
        }
        else
        {
            // 资源不存在
            LOG(WARNING, _httpRequest.path + " Not Found");
            _httpResponse.status_code = NOT_FOUND;
            goto END;
        }

        // 读取文件后缀
        pos = _httpRequest.path.rfind('.');
        if (pos== std::string::npos)
        {
            _httpRequest.suffix = ".html"; // 没找到就默认设置为html
        }
        else
        {
            _httpRequest.suffix = _httpRequest.path.substr(pos);
        }

        if(_httpRequest.cgi)
        {
            _httpResponse.status_code = ProcessCgi();
        }
        else
        {
            // 1.至此，目标网页一定是存在的
            // 2.返回并不是单单返回网页，而是要构建HTTP响应 // TODO
            _httpResponse.status_code = ProcessNonCgi();
        }

    END:
        if(_httpResponse.status_code != OK)
        {

        }
        return;
    }

    void SendResponse()
    {
        // 分多次发和把所有内容合成一个字符串一次性发送是没区别的
        // send只是把内容拷贝到发送缓冲区中
        // 具体什么时候发，一次性发多少，是由TCP决定的
        send(_sock, _httpResponse.status_line.c_str(), _httpResponse.status_line.size(), 0);
        for(auto& str : _httpResponse.response_header)
        {
            send(_sock, str.c_str(), str.size(), 0);
        }
        send(_sock, _httpResponse.blank.c_str(), _httpResponse.blank.size(), 0);
        sendfile(_sock, _httpResponse.fd, nullptr, _httpResponse.fSize); // TODO 待整理

        close(_httpResponse.fd);
    }

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

    // 处理静态网页
    int ProcessNonCgi()
    {
        _httpResponse.fd = open(_httpRequest.path.c_str(), O_RDONLY);
        if(_httpResponse.fd >= 0)
        {
            // Status_Line
            _httpResponse.status_line = HTTP_VERSION;
            _httpResponse.status_line += " ";
            _httpResponse.status_line += std::to_string(_httpResponse.status_code);
            _httpResponse.status_line += " ";
            _httpResponse.status_line += Util::Code2Desc(_httpResponse.status_code);
            _httpResponse.status_line += LINE_END;

            // Header
            std::string header_line = "Content-Type: ";
            header_line += Util::Suffix2Desc(_httpRequest.suffix);
            header_line += LINE_END;
            _httpResponse.response_header.push_back(header_line);

            header_line = "Content-Length: ";
            header_line += std::to_string(_httpResponse.fSize);
            header_line += LINE_END;
            _httpResponse.response_header.push_back(header_line);
            return OK;
        }

        return NOT_FOUND;
    }

    int ProcessCgi()
    {
        std::string &bin = _httpRequest.path;

        // 父子间通信用匿名管道 // TODO 待整理
        int input[2]; // 父进程读
        int output[2]; // 父进程写

        if(pipe(input) < 0)
        {
            LOG(ERROR, "Pipe Input Error");
            return NOT_FOUND;
        }

        if(pipe(output) < 0)
        {
            LOG(ERROR, "Pipe Output Error");
            return NOT_FOUND;
        }

        pid_t id = fork();
        if(id == 0) // Child
        {
            close(output[1]);
            close(input[0]);

            // 子进程如何知道方法是什么? // TODO
            std::string methodEnv = "METHOD=";
            methodEnv += _httpRequest.method;
            putenv((char *)methodEnv.c_str());

            // GET带参通过环境变量导入子进程 // TODO 待整理
            if(_httpRequest.method == "GET")
            {
                std::string argEnv = "ARG=";
                argEnv += _httpRequest.arg;
                putenv((char *)argEnv.c_str());

                LOG(INFO, "GET Method, Add ARG");
            }
            else if (_httpRequest.method == "POST")
            {
                std::string contentLength_Env = "CLENGTH=";
                contentLength_Env += std::to_string(_httpRequest.content_length);
                putenv((char *)contentLength_Env.c_str());

                LOG(INFO, "POST Method, Add Content_Length");
            }
            else
            {
                // Do Nothing
            }

            // 进程替换之后，子进程如何得知，对应的读写文件描述符是多少呢？ // TODO
            // 虽然替换后子进程不知道对应读写fd，但是一定知道0 && 1
            // 此时不需要知道读写fd了，只需要读0写1即可
            // 在exec*执行前，dup2重定向
            dup2(input[1], 1);
            dup2(output[0], 0);

            execl(bin.c_str(), bin.c_str(), nullptr); // TODO 待整理 #23

            exit(5);
        }
        else if(id < 0)
        {
            LOG(ERROR, "Fork Error");
            return NOT_FOUND;
        }
        else // Parent
        {
            close(output[0]);
            close(input[1]);

            if(_httpRequest.method == "POST")
            {
                // 不能确保一次性就能写完，所以
                const char *start = _httpRequest.request_body.c_str();
                int size = 0, total = 0;
                while (total < _httpRequest.request_body.size() &&
                    (size = write(output[1], start + total, _httpRequest.request_body.size() - total) > 0)) // TODO 此处优化考虑整理
                {
                    total += size;
                }
            }
    
            waitpid(id, nullptr, 0);
    
            close(output[1]);
            close(input[0]);
        }

        return OK;
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