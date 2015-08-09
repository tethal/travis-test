#ifndef HTTP2_TEST_UTIL_H_
#define HTTP2_TEST_UTIL_H_

#include <cassert>
#include "gtest/gtest.h"

//TODO move this somewhere else
#define HEX_CHAR(c)  (((c) | 0x20) % 39 - 9)
std::vector<uint8_t> hexDecode(const std::string &str) {
    std::vector<uint8_t> result;
    for (auto i = str.begin(), e = str.end(); i != e; ) {
        uint8_t b = HEX_CHAR(*i++) << 4;
        assert(i != e);
        b |= HEX_CHAR(*i++);
        result.push_back(b);
    }
    return result;
}
#undef HEX_CHAR

#define GTEST_MAIN                                  \
int main(int argc, char *argv[]) {                  \
    ::testing::InitGoogleTest(&argc, argv);         \
    return RUN_ALL_TESTS();                         \
}

#endif /* HTTP2_TEST_UTIL_H_ */
