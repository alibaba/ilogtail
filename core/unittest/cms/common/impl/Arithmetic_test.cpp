//
// Created by 韩呈杰 on 2024/1/9.
//
#ifdef _MSC_VER
// 4018 - 有符号、无符号不匹配
#pragma warning(disable: 4018)
#endif
#include <gtest/gtest.h>
#include "common/Arithmetic.h"

TEST(CommonArithmeticTest, DeltaUnsigned) {
    EXPECT_EQ(Delta((uint32_t)1, std::numeric_limits<uint32_t>::max() - 1), (uint32_t)2);
    EXPECT_EQ(Delta((uint32_t)0, std::numeric_limits<uint32_t>::max() - 1), (uint32_t)1);
    EXPECT_EQ(Delta<uint32_t>(1, (uint32_t)(0)), (uint32_t)1);
    EXPECT_EQ(Delta(1, (int16_t)(0)), 1);

    EXPECT_EQ(Increase(0, 1), 1);
    EXPECT_EQ(Increase(0, -1), 0);
}

TEST(CommonArithmeticTest, DeltaSigned) {
    EXPECT_EQ(Delta<double>(1, 0), (double)1);
    EXPECT_EQ(Delta<int>(-1, 1),0);
    EXPECT_EQ(Delta<double>(-1, 0), (uint32_t)0);

    auto n = Increase<double, double>(0, 1);
    EXPECT_EQ(n, (double)1);
    EXPECT_EQ(Increase<int>(1, -1), 0);
    EXPECT_EQ(Increase<double>(0, -1), (double)0);
}

TEST(CommonArithmeticTest, DeltaDifferentType) {
    EXPECT_EQ(Delta((int)1, (double)0), (double)1);
    EXPECT_EQ(Increase((double)0, (int)1), (double)1);
}
