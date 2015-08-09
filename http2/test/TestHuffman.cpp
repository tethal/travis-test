#include "util.h"
#include "Huffman.h"

//Using macros so that failed tests display correct line number
#define TEST_DECODE(expected, src) {                                                        \
    const std::vector<uint8_t> data = hexDecode(src);                                       \
    EXPECT_EQ(expected, Huffman::decode(data.data(), data.size()));                         \
}

//Using macros so that failed tests display correct line number
#define TEST_THROWS(src) {                                                                  \
    const std::vector<uint8_t> data = hexDecode(src);                                       \
    ASSERT_THROW(Huffman::decode(data.data(), data.size()), Huffman::InvalidEncoding);      \
}

TEST(HuffmanTest, TestName) {
    TEST_DECODE("!", "FE3F");
    TEST_DECODE("!0", "FE01");
    TEST_DECODE("00", "003F");
    TEST_DECODE("000", "0001");
    TEST_DECODE("abc", "1C64");
    TEST_DECODE("abb", "1C71FF");
    TEST_DECODE("\x7F", "FFFFFFCF");
    TEST_THROWS("FFFFFFC0");
}

//TODO more tests

GTEST_MAIN
