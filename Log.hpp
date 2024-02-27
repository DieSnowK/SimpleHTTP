#pragma once

#include <iostream>
#include <string>
#include <ctime>

#define INFO    1
#define WARNING 2
#define ERROR   3
#define FATAL   4

#define LOG(level, msg) Log(#level, msg, __FILE__, __LINE__) // TODO Write

void Log(std::string level, std::string msg, std::string file_name, int line)
{
    std::cout << "[" << level << "][" << time(nullptr) << "][" << msg << "][" << file_name << "]["<< line << "]" << std::endl;
}
