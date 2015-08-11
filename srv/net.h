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

class Socket final {
public:
    Socket(int socket) : socket(socket) {
    }

    Socket(const Socket &s) = delete;

    Socket(Socket &&s) noexcept(true) : socket(s.socket) {
        s.socket = INVALID_SOCKET;
    }

    ~Socket() {
        close();
    }

    Socket &operator=(const Socket &s) = delete;

    Socket &operator=(Socket &&s) noexcept(true) {
        socket = s.socket;
        s.socket = INVALID_SOCKET;
        return *this;
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

    operator int() const {
        return socket;
    }

private:
    int socket;
};

class Server final : private WsaContext {
public:

    using Handler = std::function<void(Socket &&)>;

    Server(uint32_t addr, uint16_t port, Handler handler) : socket(createSocket()), running(true), handler(handler) {
        socket.bind(addr, port);
        socket.listen(10);
        thread = std::thread(std::bind(&Server::threadFunc, this));
    }

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    ~Server() {
        running = false;
        socket.close();
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
            int clientSocket = ::accept(socket, (struct sockaddr*) &clientAddr, &len);

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
    Socket socket;
    std::thread thread;
    Handler handler;
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
