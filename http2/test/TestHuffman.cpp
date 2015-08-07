#include "gmock/gmock.h"
#include "Huffman.h"

static uint8_t hexChar(char c) {
    return (c | 0x20) % 39 - 9;
}

std::vector<uint8_t> hexDecode(const char *str) {
    std::vector<uint8_t> result;

    while (*str) {
        uint8_t b = hexChar(*str++) << 4;
        assert(*str != 0);
        b |= hexChar(*str++);
        result.push_back(b);
    }

    return result;
}

class HuffmanTest : public ::testing::Test {
protected:
    HuffmanTest() {}
};

#define TEST_DECODE(expected, src)      {std::vector<uint8_t> data = hexDecode(src);  EXPECT_EQ(expected, Huffman::decode(ByteArrayReader(data)));}

TEST_F(HuffmanTest, TestName) {
    TEST_DECODE("!", "FE3F");
    TEST_DECODE("!0", "FE01");
    TEST_DECODE("00", "003F");
    TEST_DECODE("000", "0001");
    TEST_DECODE("abc", "1C64");
    TEST_DECODE("abb", "1C71FF");
    TEST_DECODE("\x7F", "FFFFFFCF");
    TEST_DECODE("\x7F", "FFFFFFC0");
}

#undef TEST_DECODE

int main(int argc, char *argv[]) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
