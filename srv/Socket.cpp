#include "Socket.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <cerrno>
#define closesocket(s)          close(s)
#endif

SocketException::SocketException(std::string &&msg) : msg(std::move(msg)) {
#ifdef WIN32
    errorCode = WSAGetLastError();
#else
    errorCode = errno
#endif
}

SocketException::SocketException(std::string &&msg, int errorCode) : msg(std::move(msg)), errorCode(errorCode) {
}

std::ostream& operator<<(std::ostream &out, const SocketException &e) {
    return out << e.msg << ": error " << e.errorCode;
}

Socket::Socket() : socket(::socket(AF_INET, SOCK_STREAM, 0)) {
    if (socket == invalidSocket) {
        throw SocketException("socket() failed");
    }
}

void Socket::close() {
    if (socket != invalidSocket) {
        closesocket(socket);
        socket = invalidSocket;
    }
}

void Socket::bind(uint32_t addr, uint16_t port) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(addr);
    serverAddr.sin_port = htons(port);
    if (::bind(socket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) != 0) {
        throw SocketException("bind() failed");
    }
}

void Socket::connect(const std::string &hostName, uint16_t port) {
    struct hostent *host = gethostbyname(hostName.c_str());
    if (!host) {
        throw SocketException("gethostbyname() failed");
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr, host->h_addr, host->h_length);
    serverAddr.sin_port = htons(port);

    if (::connect(socket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) != 0) {
        throw SocketException("connect() failed");
    }
}

void Socket::listen(int backlog) {
    if (::listen(socket, backlog) != 0) {
        throw SocketException("listen() failed");
    }
}

int Socket::read(void *data, int maxLength) {
#ifdef WIN32
    int r = ::recv(socket, static_cast<char *>(data), maxLength, 0);
#else
    int r = ::read(socket, data, maxLength);
#endif
    if (r < 0) {
        throw SocketException("read() failed");
    }
    return r;
}

int Socket::write(const void *data, int length) {
#ifdef WIN32
    int w = ::send(socket, static_cast<const char *>(data), length, 0);
#else
    int w = ::write(socket, data, length);
#endif
    if (w < 0) {
        throw SocketException("write() failed");
    }
    return w;
}

#ifdef WIN32
Socket::WsaContext::WsaContext() {
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
        throw SocketException("WSAStartup failed", ret);
    }
}

Socket::WsaContext::~WsaContext() {
    WSACleanup();
}
#endif
