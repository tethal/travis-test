#ifndef HTTP2_HUFFMAN_H_
#define HTTP2_HUFFMAN_H_

#include <string>
#include <stdint.h>
#include <vector>

class ByteArrayReader {
public:
    ByteArrayReader(const uint8_t *src, size_t length) {
        data = src;
        end = data + length;
    }

    ByteArrayReader(std::vector<uint8_t> &vector) {
        data = &vector[0];
        end = data + vector.size();
    }

    bool available() {
        return data < end;
    }

    uint8_t read() {
        return *data++;
    }

private:
    const uint8_t *data;
    const uint8_t *end;
};

class Huffman {
public:
    static std::string decode(ByteArrayReader src);
};

#endif /* HTTP2_HUFFMAN_H_ */
