#include "Socket.h"
#include <iostream>
#include <vector>

std::string request(
        "GET /?gfe_rd=cr&ei=wzPLVZ_5Isak8wfhxoPADw&gws_rd=cr HTTP/1.1\r\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n"
        "Host: www.google.com\r\n"
        "Accept-Language: en-us\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "\r\n");

int main() {
    try {
        Socket socket;
        socket.connect("www.google.cz", 80);
        int written = socket.write(request.c_str(), request.length());
        std::cout << "Sent: " << written << std::endl;

        std::vector<char> buffer(10000);
        int len = socket.read(buffer.data(), buffer.capacity());
        std::cout << "Recv: " << len << std::endl;
        std::string response(&buffer[0], len);
        std::cout << response << std::endl;
    } catch (SocketException &e) {
        std::cout << e << std::endl;
    }
    return 0;
}
