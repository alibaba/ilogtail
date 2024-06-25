//
// Created by 韩呈杰 on 2023/11/19.
//
#include <gtest/gtest.h>
#include "common/Singleton.h"
#include "common/Logger.h"

#define SINGLE_FLAG_UNIT_TEST 1

struct tagA {
    static int count;
    tagA() {
        count++;
        LogInfo("{}({})", __FUNCTION__, count);
    }
    virtual ~tagA() {
        count--;
        LogInfo("{}({})", __FUNCTION__, count);
    }
};
int tagA::count = 0;

struct tagB: tagA {
    ~tagB() override {
        LogInfo("{}()", __FUNCTION__);
    }
};

TEST(CommonSingletonTest, Instance) {
    EXPECT_NE(nullptr, (Singleton<tagA, SINGLE_FLAG_UNIT_TEST>::Instance()));
    EXPECT_EQ(1, tagA::count);

    destroySingletons(SINGLE_FLAG_UNIT_TEST);
    EXPECT_EQ(0, tagA::count);
}

TEST(CommonSingletonTest, cast) {
    EXPECT_NE(nullptr, (Singleton<tagA, SINGLE_FLAG_UNIT_TEST>::Instance()));
    EXPECT_EQ(1, tagA::count);

    EXPECT_EQ(nullptr, (Singleton<tagA, SINGLE_FLAG_UNIT_TEST>::cast<tagB>()));
    delete Singleton<tagA, SINGLE_FLAG_UNIT_TEST>::swap(new tagB);
    EXPECT_NE(nullptr, (Singleton<tagA, SINGLE_FLAG_UNIT_TEST>::cast<tagB>()));

    destroySingletons(SINGLE_FLAG_UNIT_TEST);
    EXPECT_EQ(0, tagA::count);
}