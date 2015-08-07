#include "Huffman.h"
#include <cassert>
#include <cstdio>

#include "HuffmanTables.inc"

class Window {
public:
    explicit Window(ByteArrayReader &byteArrayReader) : byteArrayReader(byteArrayReader) {
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

    bool consume2(int cnt) {
        bool b = bits - padded < cnt;
        window <<= cnt;
        bits -= cnt;
        return b;
    }

    bool available() {
        return byteArrayReader.available() || window != ((1 << bits) - 1) << (16 - bits);
    }

private:
    void fill(int cnt) {
        assert(cnt <= 8);
        while (bits < cnt) {        //=> bits < 8
            uint8_t b;
            if (byteArrayReader.available()) {
                b = byteArrayReader.read();
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
    ByteArrayReader &byteArrayReader;
};

std::string Huffman::decode(ByteArrayReader src) {
    Window win(src);

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
                break;
            }
            if (win.consume2(len)) {
                printf("EEEEEE: %s\n", str.c_str());
            }
            str += (char) symbol;
            currentTable = TABLES[0];
            bits = 8;
        }
    }

    return str;
}
