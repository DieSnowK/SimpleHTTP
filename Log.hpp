#pragma once
#include <iostream>
#include <string>
#include <ctime>

#define DEBUG   0
#define INFO    1
#define WARNING 2
#define ERROR   3
#define FATAL   4

// 每次调用Log()都要手动输入__FILE__和__LINE__，很麻烦
// 所以用宏代替函数，减少固定的传参输入
// 但是level是int类型，无法传给string
// #可以把一个宏参数变成对应的字符串
#define LOG(level, msg) Log(#level, msg, __FILE__, __LINE__)

void Log(std::string level, std::string msg, std::string file_name, int line)
{
#ifndef DEBUG_SHOW
    if(level == "DEBUG")
    {
        return;
    }
#endif
    std::cout << "[" << level << "][" << time(nullptr) << "][" << msg << "][" << file_name << "]["<< line << "]" << std::endl;
}
