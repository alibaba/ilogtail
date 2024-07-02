//
// Created by 韩呈杰 on 2023/2/9.
//
#include <gtest/gtest.h>
#include "common/Random.h"
#include "common/Common.h"
#include "common/StringUtils.h"

TEST(CommonSystemRandom, Rand) {
    // 需要观察，两次运行不能产生相同的值
    Random<int> randInt{1, 10};
    for (int i = 0; i < 20; i++) {
        int n = randInt();
        std::cout << n << " ";
        EXPECT_GE(n, 1);
        EXPECT_LE(n, 10);
    }
    std::cout << std::endl;
    std::cout << common::typeName(&randInt._dist) << std::endl;
    EXPECT_TRUE(Contain(common::typeName(&randInt._dist), "uniform_int_distribution"));
}

TEST(CommonSystemRandom, RandError) {
    Random<int> randInt(10, 1);
    std::cout << randInt.next() << std::endl;
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(0, randInt.next());
    }
}

TEST(CommonSystemRandom, RandFloat) {
    {
        Random<float> randInt(1, 10);
        std::cout << randInt.next() << std::endl;
        for (int i = 0; i < 10; i++) {
            float v = randInt.next();
            EXPECT_GE(v, 1.0);
            EXPECT_LT(v, 10.0);
        }
        std::cout << common::typeName(&randInt._dist) << std::endl;
        EXPECT_TRUE(Contain(common::typeName(&randInt._dist), "uniform_real_distribution"));
    }
    {
        Random<double> randInt(1, 10);
        std::cout << randInt.next() << std::endl;
        for (int i = 0; i < 10; i++) {
            double v = randInt.next();
            EXPECT_GE(v, (double) 1.0);
            EXPECT_LT(v, (double) 10.0);
        }
    }
    {
        Random<long double> randInt(1, 10);
        std::cout << randInt.next() << std::endl;
        for (int i = 0; i < 10; i++) {
            long double v = randInt.next();
            EXPECT_GE(v, (long double) 1.0);
            EXPECT_LT(v, (long double) 10.0);
        }
    }
}
