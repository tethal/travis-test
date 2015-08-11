#include "net.h"
#include "util.h"

#if 0
#include <openssl/ssl.h>
#include <openssl/err.h>

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
int alpnCallback(SSL *ssl,
        const unsigned char **out,
        unsigned char *outlen,
        const unsigned char *in,
        unsigned int inlen,
        void *arg) {

    printf("ALPN: ");
    unsigned int offset = 0;
    const unsigned char *found = NULL;
    while (offset < inlen) {
        int len = in[offset++];
        if (memcmp("h2", in + offset, len) == 0) {
            found = in + offset;
        }
        printf("'%.*s', ", len, in + offset);
        offset += len;
    }
    if (found == NULL) {
        printf("h2 not found\n");
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    printf("found h2\n");
    *out = found;
    *outlen = found[-1];
    return SSL_TLSEXT_ERR_OK;
}

SSL_CTX *initSsl() {
    SSL_load_error_strings();
    SSL_library_init();

    SSL_CTX *sslCtx = SSL_CTX_new(TLSv1_2_server_method());
    if (sslCtx == NULL) {
        throw "Unable to create context";
    }
    if (SSL_CTX_use_certificate_file(sslCtx, "c:\\projects\\bb\\cert.cer", SSL_FILETYPE_PEM) <= 0) {
        throw "Unable to load certificate";
    }
    if (SSL_CTX_use_PrivateKey_file(sslCtx, "c:\\projects\\bb\\key.pem", SSL_FILETYPE_PEM) <= 0) {
        throw "Unable to load private key";
    }
    if (!SSL_CTX_check_private_key(sslCtx)) {
        throw "Private key does not match the certificate";
    }

    SSL_CTX_set_options(sslCtx, SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
/*
    | SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE |
            (SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|\
                    SSL_OP_NO_TLSv1|SSL_OP_NO_TLSv1_1)
    );*/
    SSL_CTX_set_alpn_select_cb(sslCtx, alpnCallback, NULL);
    //SSL_CTX_set_cipher_list(sslCtx, "ECDHE+AESGCM:!ECDSA");
    SSL_CTX_set_cipher_list(sslCtx, "ECDHE:DHE");
    SSL_CTX_set_tmp_ecdh(sslCtx, EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    return sslCtx;
}

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

void thrd(void* arg) {
    DWORD thread = GetCurrentThreadId();
    SSL *ssl = (SSL*) arg;

    printf("Thread %lu started\n", thread);
    try {
        handleClient(ssl);
    } catch (const char *e) {
        printf("Thread %lu error: %s\n", thread, e);
        ERR_print_errors_fp(stdout);
    }
    printf("Thread %lu exiting\n", thread);

    int socket = SSL_get_fd(ssl);
    SSL_free(ssl);
    closesocket(socket);
}

void acceptConnection(SSL_CTX *sslCtx, int serverSocket) {

    SSL *ssl = SSL_new(sslCtx);

    SSL_set_fd(ssl, clientSocket);

    printf("Starting thread\n");
    std::thread t1(thrd, ssl);
    t1.detach();

}

int main() {
    try {
        printf("Init SSL\n");
        SSL_CTX *sslCtx = initSsl();
        printf("Init server\n");
        int serverSocket = initServer();
        while (true) {
            printf("Before acceptConnection\n");
            acceptConnection(sslCtx, serverSocket);
        }
        SSL_CTX_free(sslCtx);
    } catch (const char *e) {
        printf("%s\n", e);
        ERR_print_errors_fp(stdout);
    }
    return 0;
}
#endif

#define LISTEN_PORT 4430


class Client {
public:
    Client(Socket &&s) : s(std::move(s)) {
        log("Client created\n");
    }

    ~Client() {
        log("Client done\n");
    }

    void handleClient() {
        //TODO
    }

private:
    Socket s;
};

int main() {
    try {
        //ServerSocket s(LISTEN_PORT, std::bind(&x, 123, std::placeholders::_1));
        //ServerSocket s(LISTEN_PORT, &x);
        ServerSocket s(LISTEN_PORT, [](Socket &&socket){
            Client *c = new Client(std::move(socket));
            std::thread([=]() {
                c->handleClient();
                delete c;
            }).detach();
        });
        getchar();
        log("Shutting down\n");
    } catch (std::exception &e) {
        log("Caught exception!\n");
    }
    log("Done.\n");
    return 0;
}
