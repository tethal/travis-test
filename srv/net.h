#ifndef SRV_NET_H_
#define SRV_NET_H_

#include <cstdint>
#include <exception>
#include <functional>
#include "util.h"

#if 0

class Server final {
public:

    using ClientFactory = std::function<void(Socket &&)>;

    Server(uint32_t addr, uint16_t port, ClientFactory clientFactory) : socket(createSocket()), running(true), clientFactory(clientFactory) {
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
    }
};

#endif

#endif /* SRV_NET_H_ */
