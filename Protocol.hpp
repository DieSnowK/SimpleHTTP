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
#define PAGE_404 "404.html"

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
        if (_request.method != "GET" && _request.method != "POST")
        {
            // 非法请求
            _response.status_code = NOT_FOUND; // TODO
            LOG(WARNING, "Method is not right");
            goto END;
        }

        if (_request.method == "GET")
        {
            // GET可能带参数，也可能不带参数，要区分出来
            if (_request.uri.find('?') != std::string::npos)
            {
                Util::CutString(_request.uri, _request.path, _request.arg, "?");
                _request.cgi = true;
            }
            else
            {
                _request.path = _request.uri;
            }
        }
        else if(_request.method == "POST")
        {
            _request.cgi = true;
            _request.path = _request.uri;
        }
        else
        {
            // Do Nothing
        }

        _request.path.insert(0, WEB_ROOT); // 从根目录开始

        LOG(DEBUG, _request.path);
        LOG(DEBUG, _request.arg);

        // 如果访问的是根目录，则默认访问主页
        if(_request.path[_request.path.size() - 1] == '/')
        {
            _request.path += HOME_PAGE;
        }

        LOG(DEBUG, _request.path);

        // 确认访问资源是否存在
        struct stat st;
        if(stat(_request.path.c_str(), &st) == 0) // TODO  待整理stat()
        {
            // 访问的是否是一个具体资源？
            if(S_ISDIR(st.st_mode))
            {
                // 请求的是一个目录，需要特殊处理 -- 改为访问该目录下主页
                // 虽然是目录，但是绝对不会以/结尾
                _request.path += "/";
                _request.path += HOME_PAGE;

                // 上面更改了path指向，所以重新获取文件状态
                stat(_request.path.c_str(), &st);
            }

            // 是否是一个可程序程序？
            if (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)
            {
                _request.cgi = true; // TODO CGI标志位感觉有多余
            }

            _response.fSize = st.st_size;
        }
        else
        {
            // 资源不存在
            LOG(WARNING, _request.path + " Not Found");
            _response.status_code = NOT_FOUND;
            goto END;
        }

        // 读取文件后缀
        pos = _request.path.rfind('.');
        if (pos== std::string::npos)
        {
            _request.suffix = ".html"; // 没找到就默认设置为html
        }
        else
        {
            _request.suffix = _request.path.substr(pos);
        }

        if(_request.cgi)
        {
            _response.status_code = ProcessCgi();
        }
        else
        {
            // 1.至此，目标网页一定是存在的
            // 2.返回并不是单单返回网页，而是要构建HTTP响应 // TODO
            _response.status_code = ProcessNonCgi();
        }

    END:
        if(_response.status_code != OK)
        {
            BuildResponseHelper();
        }
        return;
    }

    void SendResponse()
    {
        // 分多次发和把所有内容合成一个字符串一次性发送是没区别的
        // send只是把内容拷贝到发送缓冲区中
        // 具体什么时候发，一次性发多少，是由TCP决定的
        send(_sock, _response.status_line.c_str(), _response.status_line.size(), 0);
        for(auto& str : _response.response_header)
        {
            send(_sock, str.c_str(), str.size(), 0);
        }
        send(_sock, _response.blank.c_str(), _response.blank.size(), 0);
        sendfile(_sock, _response.fd, nullptr, _response.fSize); // TODO 待整理

        close(_response.fd);
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
        Util::ReadLine(_sock, _request.request_line);
        _request.request_line.resize(_request.request_line.size() - 1); // 去掉结尾换行符
        LOG(DEBUG, _request.request_line);
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
                _request.blank = line;
                break;
            }

            line.resize(line.size() - 1); // 去掉结尾换行符
            _request.request_header.push_back(line);
            
            LOG(DEBUG, line); 
        }
    }

    void ParseRequestLine()
    {
        std::stringstream ss(_request.request_line); // TODO 可整理
        ss >> _request.method >> _request.uri >> _request.version;

        // TODO 可整理
        // 可能不是所有人都严格遵守标准，所以将method统一转化为大写
        std::transform(_request.method.begin(), _request.method.end(), _request.method.begin(), ::toupper);

        LOG(DEBUG, _request.method);
        LOG(DEBUG, _request.uri);
        LOG(DEBUG, _request.version);
    }

    void ParseRequestHeader()
    {
        std::string key;
        std::string value;
        for(auto& str : _request.request_header)
        {
           if(Util::CutString(str, key, value, SEP))
           {
               _request.headerMap[key] = value;
           }
        }
    }

    bool IsRecvRequestBody()
    {
        std::string& method = _request.method;
        if(method == "POST")
        {
            _request.content_length = atoi(_request.headerMap["Content-Length"].c_str());
            return true;
        }

        return false;
    }
    
    void RecvRequestBody()
    {
        if(IsRecvRequestBody())
        {
            size_t length = _request.content_length;
            auto& body = _request.request_body;

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
        _response.fd = open(_request.path.c_str(), O_RDONLY);
        if(_response.fd >= 0)
        {
            // Status_Line
            _response.status_line = HTTP_VERSION;
            _response.status_line += " ";
            _response.status_line += std::to_string(_response.status_code);
            _response.status_line += " ";
            _response.status_line += Util::Code2Desc(_response.status_code);
            _response.status_line += LINE_END;

            // Header
            std::string header_line = "Content-Type: ";
            header_line += Util::Suffix2Desc(_request.suffix);
            header_line += LINE_END;
            _response.response_header.push_back(header_line);

            header_line = "Content-Length: ";
            header_line += std::to_string(_response.fSize);
            header_line += LINE_END;
            _response.response_header.push_back(header_line);
            return OK;
        }

        return NOT_FOUND;
    }

    int ProcessCgi()
    {
        int code = 0; // 退出码
        std::string &bin = _request.path;

        // 父子间通信用匿名管道 // TODO 待整理
        int input[2]; // 父进程读
        int output[2]; // 父进程写

        if(pipe(input) < 0)
        {
            LOG(ERROR, "Pipe Input Error");
            code = NOT_FOUND;
            return code;
        }

        if(pipe(output) < 0)
        {
            LOG(ERROR, "Pipe Output Error");
            code = NOT_FOUND;
            return code;
        }

        pid_t id = fork();
        if(id == 0) // Child
        {
            close(output[1]);
            close(input[0]);

            // 子进程如何知道方法是什么? // TODO
            std::string methodEnv = "METHOD=";
            methodEnv += _request.method;
            putenv((char *)methodEnv.c_str());

            // GET带参通过环境变量导入子进程 // TODO 待整理
            if(_request.method == "GET")
            {
                std::string argEnv = "ARG=";
                argEnv += _request.arg;
                putenv((char *)argEnv.c_str());

                LOG(INFO, "GET Method, Add ARG");
            }
            else if (_request.method == "POST")
            {
                std::string contentLength_Env = "CLENGTH=";
                contentLength_Env += std::to_string(_request.content_length);
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
            code = NOT_FOUND;
            return code;
        }
        else // Parent
        {
            close(output[0]);
            close(input[1]);

            if(_request.method == "POST")
            {
                // 不能确保一次性就能写完，所以
                const char *start = _request.request_body.c_str();
                int size = 0, total = 0;
                while (total < _request.request_body.size() &&
                    (size = write(output[1], start + total, _request.request_body.size() - total) > 0)) // TODO 此处优化考虑整理
                {
                    total += size;
                }
            }

            // 读取CGI子进程的处理结果
            char ch = 'K';
            while(read(input[0], &ch, 1) > 0)
            {
                // CGI执行完之后的结果，并不可以直接返回给浏览器，因为这部分内容只是响应正文
                _response.response_body += ch;
            }

            int status = 0;
            pid_t ret = waitpid(id, &status, 0);
            if(ret == id)
            {
                if(WIFEXITED(status))
                {
                    if(WEXITSTATUS(status) == 0)
                    {
                        code = OK;
                    }
                    else
                    {
                        code = NOT_FOUND;
                    }
                }
                else
                {
                    code = NOT_FOUND;
                }
            }

            close(output[1]);
            close(input[0]);
        }

        return OK;
    }

    void BuildResponseHelper()
    {
        // 此处status_line是干净的，没有内容的
        _response.status_line += HTTP_VERSION;
        _response.status_line += " ";
        _response.status_line += std::to_string(_response.status_code);
        _response.status_line += " ";
        _response.status_line += Util::Code2Desc(_response.status_code);

        switch(_response.status_code)
        {
        case 404:
            HandlerNotFound();
            break;
        default:
            break;
        }
    }

    void HandlerNotFound()
    {
        // 给用户返回对应的404页面
    }

private:
    int _sock;
    HttpRequest _request;
    HttpResponse _response;
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