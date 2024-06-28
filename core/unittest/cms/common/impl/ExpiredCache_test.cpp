//
// Created by 韩呈杰 on 2023/7/7.
//
#include <gtest/gtest.h>
#include <thread>
#include "common/ExpiredCache.h"

TEST(CommonExpiredCacheTest, Normal) {
    int count = 0;
    auto supply = [&]() {
        std::stringstream ss;
        ss << (count++);
        return ss.str();
    };

    ExpiredCache<std::string> cache(supply, std::chrono::milliseconds{1});
    EXPECT_EQ(cache(), "0");
    EXPECT_EQ(cache(), "0");

    // 超时更新
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
    EXPECT_EQ(cache(), "1");
}