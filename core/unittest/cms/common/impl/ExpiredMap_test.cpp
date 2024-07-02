//
// Created by 韩呈杰 on 2023/8/4.
//
#include <gtest/gtest.h>
#include "common/ExpiredMap.h"

#include <thread>

using namespace std::chrono;

TEST(CommonExpiredMapTest, Normal) {
    auto fnCompute = [](const int &k, std::map<int, int> &v) {
        v[k] = k;
        return true;
    };
    ExpiredMap<int, int> data{microseconds{1000}};
    EXPECT_EQ(data.Size(), size_t(0));
    EXPECT_TRUE(data.IsEmpty());

    EXPECT_EQ(data.Compute(1, fnCompute).v, 1);
    EXPECT_EQ(size_t(1), data.Size());
    EXPECT_FALSE(data.IsEmpty());

    // 未过期，新值无效
    auto fnCompute1 = [](const int &k, std::map<int, int> &v) {
        v[k] = 0;
        return true;
    };
    EXPECT_EQ(data.Compute(1, fnCompute1).v, 1);
    EXPECT_EQ(size_t(1), data.Size());
    EXPECT_FALSE(data.IsEmpty());

    std::this_thread::sleep_for(microseconds{1100});

    auto fnCompute2 = [](const int &k, std::map<int, int> &v) {
        v[k] = k * 2;
        return true;
    };
    EXPECT_EQ(data.Compute(1, fnCompute2).v, 2);
    EXPECT_EQ(size_t(1), data.Size());
    EXPECT_FALSE(data.IsEmpty());
}
