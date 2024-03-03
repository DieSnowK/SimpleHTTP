#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>
using namespace std;

int main()
{
    // 此时子进程标准输出已经重定向，想看打印只能从标准错误输出 #27 // TODO
    cerr << getenv("METHOD") << endl;

    std::string method = getenv("METHOD");
    std::string argStr;

    if (method == "GET")
    {
        argStr = getenv("ARG");
    }
    else if(method == "POST")
    {
        // CGI如何得知需要从标准输入读取多少字节呢？
        int content_length = atoi(getenv("CLENGTH"));

        char ch = 'K';
        while(content_length--)
        {
            read(0, &ch, 1);
            argStr.push_back(ch);
        }
    }
    else
    {

    }

    cerr << "++++++++++++++++++++++++++";

    return 0;
}