#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>

bool GetQueryString(std::string &query_string)
{
    bool result = false;
    std::string method = getenv("METHOD");
    if(method == "GET"){
        query_string = getenv("QUERY_STRING");
        result = true;
    }
    else if(method == "POST"){
        //CGI如何得知需要从标准输入读取多少个字节呢？？
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        char c = 0;
        while(content_length){
            read(0, &c, 1);
            query_string.push_back(c);
            content_length--;
        }
        result = true;
    }
    else{
        result = false;
    }
    return result;
}

void CutString(std::string &in, const std::string &sep, std::string &out1, std::string &out2)
{
    auto pos = in.find(sep);
    if(std::string::npos != pos){
        out1 = in.substr(0, pos);
        out2 = in.substr(pos+sep.size());
    }
}

