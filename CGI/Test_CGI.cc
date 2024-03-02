#include <iostream>
#include <cstdlib>
using namespace std;

int main()
{
    // 此时子进程标准输出已经重定向，想看打印只能从标准错误输出 #27 // TODO
    cerr << getenv("METHOD") << endl;
}