#ifndef SRV_NET_H_
#define SRV_NET_H_

#include <cstdint>
#include <exception>
#include <functional>
#include <winsock.h>
#include "util.h"

class WsaContext {
public:
    WsaContext() {
        WSADATA wsaData;
        int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (ret != 0) {
            log("WSAStartup failed with error code %d\n", ret);
            throw std::exception();
        }
    }
    ~WsaContext() {
        WSACleanup();
    }
};

class Socket {
public:
    Socket(int socket) : socket(socket) {
        log("CREATE %d\n", socket);
    }

    Socket(const Socket &s) : socket(INVALID_SOCKET) {
        log("COPY %d\n", s.socket);
    }

    Socket(Socket &&s) : socket(s.socket) {
        s.socket = INVALID_SOCKET;
        log("MOVE %d\n", socket);
    }

    Socket &operator=(const Socket &s) {
        log("COPY-ASSIGN %d\n", s.socket);
        return *this;
    }

    Socket &operator=(Socket &&s) {
        socket = s.socket;
        s.socket = INVALID_SOCKET;
        log("MOVE-ASSIGN %d\n", socket);
        return *this;
    }

    ~Socket() {
        log("CLOSE %d\n", socket);
        close();
    }

    void close() {
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
    }

    void bind(uint32_t addr, uint16_t port) {
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(addr);
        serverAddr.sin_port = htons(port);
        if (::bind(socket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) != 0) {
            log("bind() failed: %d\n", WSAGetLastError());
            throw std::exception();
        }
    }

    void listen(int backlog) {
        if (::listen(socket, backlog) != 0) {
            log("listen() failed: %d\n", WSAGetLastError());
            throw std::exception();
        }
    }

protected:
    int getSocket() const {
        return socket;
    }

private:
    int socket;
};

class ServerSocket final : private WsaContext, Socket {
public:

    ServerSocket(uint16_t port, std::function<void(Socket &)> handler) : ServerSocket(INADDR_ANY, port, handler) {
    }

    ServerSocket(uint32_t addr, uint16_t port, std::function<void(Socket &)> handler) : Socket(createSocket()), running(true), handler(handler) {
        bind(addr, port);
        listen(10);
        thread = std::thread(std::bind(&ServerSocket::threadFunc, this));
    }

    ~ServerSocket() {
        running = false;
        close();
        if (thread.joinable()) {
            thread.join();
        }
    }

private:
    void threadFunc() {
        log("Server thread started\n");
        while (running) {
            struct sockaddr_in clientAddr;
            int len = sizeof(clientAddr);
            int clientSocket = ::accept(getSocket(), (struct sockaddr*) &clientAddr, &len);

            if (clientSocket != INVALID_SOCKET) {
                log("Accepted connection from: %s\n", inet_ntoa(clientAddr.sin_addr));
                handler(Socket(clientSocket));
            } else if (running) {
                log("accept() error: %d\n", WSAGetLastError());
                break;
            }
        }
        log("Server thread finished\n");
    }

private:
    std::thread thread;
    std::function<void(Socket &)> handler;
    volatile bool running;

    static int createSocket() {
        int socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket == INVALID_SOCKET) {
            log("socket() failed: %d\n", WSAGetLastError());
            throw std::exception();
        }
        return socket;
    }
};

#endif /* SRV_NET_H_ */
