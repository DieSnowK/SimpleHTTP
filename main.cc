#include "HttpServer.hpp"

int main()
{
    TcpServer::GetInstance(8080);
    sleep(1000);
    return 0;
}
