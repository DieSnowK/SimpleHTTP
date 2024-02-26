#include <iostream>
#include <string>
#include <algorithm>

int main()
{
    std::string method = "Get";
    std::cout << "method: " << method << std::endl;

    std::transform(method.begin(), method.end(), method.begin(), ::toupper);

    std::cout << "method: " << method << std::endl;

}
