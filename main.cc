#include <iostream>
#include <memory>
#include "HttpServer.hpp"

void Usage(std::string proc)
{
    std::cout << "Usage:\n\t" << proc << " Port" << std::endl;
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        Usage(argv[0]);
        exit(4);
    }

    // daemon(1, 0);

    std::unique_ptr<HttpServer> httpServer(new HttpServer(atoi(argv[1])));
    httpServer->Init();
    httpServer->Loop();

    return 0;
}
