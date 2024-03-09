#pragma once
#include "Protocol.hpp"

class Task
{
public:
    void ProcessOn()
    {
        _handler(_sock);
    }
    
    Task()
    {}

    Task(int sock)
        : _sock(sock)
    {}

    ~Task()
    {}
private:
    int _sock;
    CallBack _handler; // …Ë÷√ªÿµ˜
};