#pragma once
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>

struct Util
{
    // 不同平台对空格的处理是不一样的，要统一处理
    static int ReadLine(int sock, std::string &out)
    {
        char ch = 'K';
        while(ch != '\n')
        {
            ssize_t s = recv(sock, &ch, 1, 0);
            if(s > 0)
            {
                // Success
                if(ch == '\r')
                {
                    // (\r\n or \r) --> \n
                    recv(sock, &ch, 1, MSG_PEEK); // 窥探，看缓冲区最前面的n个字符而不从缓冲区里拿出来
                    if(ch == '\n')
                    {
                        recv(sock, &ch, 1, 0);
                    }
                    else
                    {
                        ch = '\n';
                    }
                }

                // 1.Normal
                // 2.\n
                out.push_back(ch);
            }
            else if(s == 0)
            {
                // Done
                return 0;
            }
            else
            {
                // Error
                return -1;
            }
        }

        return out.size();
    }

    static bool CutString(const std::string& str, std::string& sub1, std::string& sub2, std::string sep)
    {
        size_t pos = str.find(sep);
        if(pos != std::string::npos)
        {
            sub1 = str.substr(0, pos);
            sub2 = str.substr(pos + sep.size());
            return true;
        }

        return false;
    }

    static std::string Code2Desc(int code)
    {
        std::string desc = "";
        switch (code)
        {
        case 200:
            desc = "OK";
            break;
        case 404:
            desc = "NOT_FOUND";
            break;
        case 400:
            desc = "BAD_REQUEST";
            break;
        case 500:
            desc = "SERVER_ERROR";
            break;
        default:
            break;
        }

        return desc;
    }

    // 后面可以考虑写进配置文件，然后load进来
    // 暂且方案先写成硬编码进源码里
    static std::string Suffix2Desc(const std::string& suffix)
    {
        static std::unordered_map<std::string, std::string> suffix2desc = {
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".jpg", "application/x-jpg"},
            {".xml", "application/xml"}
        };

        auto iter = suffix2desc.find(suffix);
        if(iter != suffix2desc.end())
        {
            return iter->second;
        }
        else
        {
            return "text/html";
        }
    }
};
