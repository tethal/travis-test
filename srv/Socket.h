#ifndef SRV_SOCKET_H_
#define SRV_SOCKET_H_

#include <cstdint>
#include <exception>
#include <ostream>
#include <string>

class Stream {
public:
    virtual ~Stream() {}
};

class SocketException : public std::exception {
public:
    SocketException(std::string &&msg);
    SocketException(std::string &&msg, int errorCode);

private:
    std::string msg;
    int errorCode;

    friend std::ostream& operator<<(std::ostream &out, const SocketException &e);
};

class Socket final : public Stream {

public:
    Socket();
    Socket(int socket) : socket(socket) {}
    Socket(const Socket &s) = delete;
    Socket(Socket &&s) noexcept(true) : socket(s.socket) {
        s.socket = invalidSocket;
    }

    ~Socket() {
        close();
    }

    Socket &operator=(const Socket &s) = delete;
    Socket &operator=(Socket &&s) noexcept(true) {
        socket = s.socket;
        s.socket = invalidSocket;
        return *this;
    }

    operator int() const {
        return socket;
    }

    void close();
    void bind(uint32_t addr, uint16_t port);
    void connect(const std::string &hostName, uint16_t port);
    void listen(int backlog);
    int read(void *data, int maxLength);
    int write(const void *data, int length);

private:
    constexpr static int invalidSocket = -1;

#ifdef WIN32
    class WsaContext final {
    public:
        WsaContext();
        ~WsaContext();
    };
    WsaContext wsaContext;
#endif

    int socket;
};

#endif /* SRV_SOCKET_H_ */
