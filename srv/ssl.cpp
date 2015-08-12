#include "ssl.h"

#if 0
bool SslInitializer::sslInitDone = false;

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
#endif
