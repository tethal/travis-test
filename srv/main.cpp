#include "ssl.h"

const char *resp = R"(HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: length

<html>
  <head>
    <title>Test!</title>
  </head>
  <body>
    Hello, world!
  </body>
</html>
)";

#if 0
void hex(const unsigned char *buf, unsigned int len) {
    for (unsigned int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
}

void hexnl(const unsigned char *buf, unsigned int len) {
    hex(buf, len);
    printf("\n");
}

//ALPN: 08 68 74 74 70 2F 31 2E 31 08 73 70 64 79 2F 33 2E 31 05 68 32 2D 31 34 02 68 32

void readFully(SSL *ssl, unsigned char *buf, int len) {
    int r = 0;
    int cnt = 0;
    while (r < len) {
        int bytes = SSL_read(ssl, buf + r, len - r);
        if (bytes == 0) {
            cnt++;
            if (cnt > 1000) {
                throw "FOFF";
            }
        } else {
            cnt = 0;
        }
        if (bytes < 0) {
            int i = SSL_get_error(ssl, bytes);
            if (i == SSL_ERROR_ZERO_RETURN) {
                throw "Closed";
            }
            printf("Thread %lu read error: %d, errno: %d\n", GetCurrentThreadId(), i, errno);
            throw "Unable to read";
        }
        r += bytes;
    }
}

const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

struct Frame {
    unsigned int len;
    unsigned char type;
    unsigned char flags;
    unsigned int streamId;
    unsigned char payload[16384];

    Frame() {
        len = 0;
        type = 0;
        flags = 0;
        streamId = 0;
    }

    Frame(unsigned char type, unsigned char flags, unsigned int streamId) {
        len = 0;
        this->type = type;
        this->flags = flags;
        this->streamId = streamId;
    }

    void read(SSL *ssl);
    void dump();
};

void Frame::read(SSL *ssl) {
    unsigned char header[9];

    readFully(ssl, header, 9);
    len = (header[0] << 16) | (header[1] << 8) | header[2];
    type = header[3];
    flags = header[4];
    streamId = (header[5] << 24) | (header[6] << 16) | (header[7] << 8) | header[8];
    if (len > sizeof(payload)) {
        throw "Frame too big";
    }
    readFully(ssl, payload, len);
}

void Frame::dump() {
    printf("Thread %lu received frame type: %02X, flags: %02X, len: %u, streamId: %08X\n  ", GetCurrentThreadId(), type, flags, len, streamId);
    hexnl(payload, len);
}

void sendFrame(SSL *ssl, const Frame &f) {
    unsigned char header[9];
    header[0] = f.len >> 16;
    header[1] = f.len >> 8;
    header[2] = f.len;
    header[3] = f.type;
    header[4] = f.flags;
    header[5] = f.streamId >> 24;
    header[6] = f.streamId >> 16;
    header[7] = f.streamId >> 8;
    header[8] = f.streamId;
    printf("Thread %lu sending: ", GetCurrentThreadId());
    hex(header, 9);
    SSL_write(ssl, header, 9);
    hexnl(f.payload, f.len);
    if (f.len > 0) {
        SSL_write(ssl, f.payload, f.len);
    }
}

void handleClient(SSL *ssl) {
    if (SSL_accept(ssl) != 1) {
        throw "SSH handshake error";
    }
    const unsigned char *alpn;
    unsigned int len;
    SSL_get0_alpn_selected(ssl, &alpn, &len);
    printf("SELECTED: %.*s\n", len, alpn);

    unsigned char buf[24];
    readFully(ssl, buf, 24);
    if (memcmp(buf, preface, 24) != 0) {
        throw "Invalid preface";
    }

    Frame frame;
    sendFrame(ssl, Frame(4, 0, 0));
    while (true) {
        frame.read(ssl);
        frame.dump();
        switch (frame.type) {
            case 4:
                if (!frame.flags) {
                    sendFrame(ssl, Frame(4, 1, 0));
                }
                break;
            default:
                printf("Unknown frame type %02X\n", frame.type);
        }
    }
}
#endif

class Client {
public:
    Client(SslWrapper &&ssl) : ssl(std::move(ssl)) {
        log("Client created\n");
    }

    ~Client() {
        log("Client done\n");
    }

    void handleClient() {
        char buf[2048];
        int bytes = SSL_read(ssl, buf, sizeof(buf));
        buf[bytes] = 0;
        printf("Received:\n%s\n", buf);
        fflush(stdout);
        SSL_write(ssl, resp, strlen(resp));
    }

private:
    SslWrapper ssl;
};

#define LISTEN_PORT 4430

int main() {
    try {
        SslServer server(INADDR_ANY, LISTEN_PORT, [](SslWrapper &&client) {
            Client *c = new Client(std::move(client));
            std::thread([=]() {
                c->handleClient();
                delete c;
            }).detach();
        });
        getchar();
        //should wait for clients - they hold indirect references to SSL_CTX, which will be freed here
        log("Shutting down\n");
    } catch (SslException &e) {
        log("Caught SSL exception!\n");
        ERR_print_errors_fp(stdout);
    } catch (std::exception &e) {
        log("Caught exception!\n");
    }
    log("Done.\n");
    return 0;
}
