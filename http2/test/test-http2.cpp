#include "gmock/gmock.h"

int x();

TEST(TestCase, TestName) {
    EXPECT_EQ(0, x());
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
