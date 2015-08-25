#include "Huffman.h"
#include <cassert>

#include "HuffmanTables.inc"

//TODO cleanup

namespace {
class Window {
public:
    Window(const uint8_t *begin, const uint8_t *end) : ptr(begin), end(end) {
        window = 0;
        bits = 0;
        padded = 0;
    }

    uint8_t peek(int cnt) {
        fill(cnt);
        return window >> (16 - cnt);
    }

    void consume(int cnt) {
        //fill(cnt);
        window <<= cnt;
        bits -= cnt;
    }

    void consume2(int cnt) {
        if (bits - padded < cnt) {
            throw Huffman::InvalidEncoding();
        }
        window <<= cnt;
        bits -= cnt;
    }

    bool available() {
        return ptr < end || window != ((1 << bits) - 1) << (16 - bits);
    }

private:
    void fill(int cnt) {
        assert(cnt <= 8);
        while (bits < cnt) {        //=> bits < 8
            uint8_t b;
            if (ptr < end) {
                b = *ptr++;
            } else {
                b = 0xFF;
                padded += 8;
            }
            window |= b << (8 - bits);
            bits += 8;
        }
    }

private:
    uint16_t window;
    int bits;
    int padded;
    const uint8_t *ptr;
    const uint8_t *end;
};
}

std::string Huffman::decode(const uint8_t *src, size_t length) {
    Window win(src, src + length);
    std::string str;

    const uint16_t *currentTable = TABLES[0];
    int bits = 8;

    while (win.available()) {
        uint16_t entry = currentTable[win.peek(bits)];
        if (entry & 0x8000) {
            win.consume(8);
            int table = (entry >> 4) & 0x1FF;
            bits = entry & 15;
            currentTable = TABLES[table];
        } else {
            int symbol = (entry >> 4) & 0x1FF;
            int len = entry & 15;
            if (symbol == 256) {
                throw InvalidEncoding();
            }
            win.consume2(len);
            str += (char) symbol;
            currentTable = TABLES[0];
            bits = 8;
        }
    }

    return str;
}
