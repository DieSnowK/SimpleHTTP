#pragma once

#include <string>
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
            out += ch;
        }

        return out.size();
    }
};