#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>
using namespace std;

bool GetQuery(std::string& out)
{
    std::string method = getenv("METHOD");

    bool ret = false;
    if (method == "GET")
    {
        out = getenv("ARG");
        ret = true;
    }
    else if (method == "POST")
    {
        // CGI如何得知需要从标准输入读取多少字节呢？
        int content_length = atoi(getenv("CLENGTH"));

        char ch = 'K';
        while (content_length--)
        {
            read(0, &ch, 1);
            out.push_back(ch);
        }
        ret = true;
    }
    else
    {
        // Do Nothing
    }

    return ret;
}

void CutString(const std::string& in, std::string& out1, std::string& out2, const std::string sep)
{
    auto pos = in.find(sep);
    if(pos != std::string::npos)
    {
        out1 = in.substr(0, pos);
        out2 = in.substr(pos + sep.size());
    }
}

int main()
{
    // 此时子进程标准输出已经重定向，想看打印只能从标准错误输出 #27 // TODO
    std::string queryStr;
    GetQuery(queryStr);

    // Test Code：x=100&y=200
    std::string arg1, arg2;
    CutString(queryStr, arg1, arg2, "&");

    std::string key1, value1, key2, value2;
    CutString(arg1, key1, value1, "=");
    CutString(arg2, key2, value2, "=");

    // 1 -> 数据给父进程
    std::cout << key1 << ":" << value1 << endl;
    std::cout << key2 << ":" << value2 << endl;

    // 2 -> DEBUG，输出到命令行
    std::cerr << "CGI: " << key1 << ":" << value1 << endl;
    std::cerr << "CGI: " << key2 << ":" << value2 << endl;

    int x = atoi(value1.c_str());
    int y = atoi(value2.c_str());

    // 可能想进行某种计算(搜索、登陆等)，想进行某种存储(注册)
    std::cout << "<html>";
    std::cout << "<head><meta charset=\"utf-8\"></head>";
    std::cout << "<body>";
    std::cout << "<h3> " << value1 << " + " << value2 << " = " << x + y << "</h3>";
    std::cout << "<h3> " << value1 << " - " << value2 << " = " << x - y << "</h3>";
    std::cout << "<h3> " << value1 << " * " << value2 << " = " << x * y << "</h3>";
    std::cout << "<h3> " << value1 << " / " << value2 << " = " << x / y << "</h3>";
    std::cout << "</body>";
    std::cout << "</html>";

    return 0;
}