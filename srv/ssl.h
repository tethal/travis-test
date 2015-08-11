#ifndef SRV_SSL_H_
#define SRV_SSL_H_

#include <memory>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "net.h"
#include "util.h"

int alpnCallback(SSL *ssl,
        const unsigned char **out,
        unsigned char *outlen,
        const unsigned char *in,
        unsigned int inlen,
        void *arg);


class SslException {
};

class SslInitializer {

protected:
    SslInitializer() {
        if (!sslInitDone) {
            SSL_load_error_strings();
            SSL_library_init();
            sslInitDone = true;
        }
    }

private:
    static bool sslInitDone;
};

class SslContextWrapper {
public:
    SslContextWrapper(SSL_CTX *ctx) : ctx(ctx) {
        if (!ctx) {
            log("SSL_CTX_new failed");
            throw SslException();
        }
    }

    ~SslContextWrapper() {
        SSL_CTX_free(ctx);
    }

    SslContextWrapper(const SslContextWrapper &) = delete;
    SslContextWrapper &operator=(const SslContextWrapper &) = delete;

    SslContextWrapper(SslContextWrapper &&) = delete;
    SslContextWrapper &operator=(SslContextWrapper &&) = delete;

    void setupKey(const char *privateKeyFileName, const char *certificateFileName) {
        if (SSL_CTX_use_certificate_file(ctx, certificateFileName, SSL_FILETYPE_PEM) <= 0) {
            log("Unable to load certificate");
            throw SslException();
        }
        if (SSL_CTX_use_PrivateKey_file(ctx, privateKeyFileName, SSL_FILETYPE_PEM) <= 0) {
            log("Unable to load private key");
            throw SslException();
        }
        if (!SSL_CTX_check_private_key(ctx)) {
            log("Private key does not match the certificate");
            throw SslException();
        }
    }

    operator SSL_CTX*() const { return ctx; }

private:
    SSL_CTX* ctx;
};

class SslWrapper {

public:
    SslWrapper(SslContextWrapper &ctx, Socket &&clientSocket) : clientSocket(std::move(clientSocket)), ssl(SSL_new(ctx)) {
        if (!ssl) {
            log("SSL_new failed\n");
            throw SslException();
        }
        SSL_set_fd(ssl, this->clientSocket);
        if (SSL_accept(ssl) != 1) {
            log("SSL handshake error\n");
            throw SslException();
        }
    }

    ~SslWrapper() {
        SSL_free(ssl);
    }

    SslWrapper(const SslWrapper &) = delete;
    SslWrapper &operator=(const SslWrapper &) = delete;

    SslWrapper(SslWrapper &&rhs) noexcept(true) : clientSocket(std::move(rhs.clientSocket)), ssl(rhs.ssl) {
        rhs.ssl = nullptr;
    }
    SslWrapper &operator=(SslWrapper &&) = delete;

    operator SSL*() const { return ssl; }

private:
    Socket clientSocket;
    SSL *ssl;
};

class SslContext : private SslInitializer {

public:
    SslContext() : ctx(SSL_CTX_new(TLSv1_2_server_method())) {
        ctx.setupKey("c:\\projects\\bb\\key.pem", "c:\\projects\\bb\\cert.cer");
        SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
//        SSL_CTX_set_alpn_select_cb(ctx, alpnCallback, NULL);
        SSL_CTX_set_cipher_list(ctx, "ECDHE:DHE");
        SSL_CTX_set_tmp_ecdh(ctx, EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    }

    ~SslContext() {
    }

    SslWrapper wrap(Socket &&clientSocket) {
        return SslWrapper(ctx, std::move(clientSocket));
    }

private:
    SslContextWrapper ctx;
};

class SslServer final {
public:

    using Handler = std::function<void(SslWrapper &&)>;

    SslServer(uint32_t addr, uint16_t port, Handler handler) : server(addr, port, std::bind(&SslServer::accept, this, std::placeholders::_1)), handler(handler) {
    }

private:
    void accept(Socket &&socket) {
        try {
            handler(sslCtx.wrap(std::move(socket)));
        } catch (SslException &e) {
            log("Caught SSL exception in accept()!\n");
            ERR_print_errors_fp(stdout);
        } catch (std::exception &e) {
            log("Caught exception in accept()!\n");
        }
    }

private:
    SslContext sslCtx;
    Server server;
    Handler handler;
};

#endif /* SRV_SSL_H_ */
