//
// Created by 韩呈杰 on 2024/1/22.
//
#include <gtest/gtest.h>
#include "common/ConfigBase.h"

struct StubConfig: public ConfigBase {
    std::string v;
    using ConfigBase::GetValue;

    std::string GetValue(const std::string &key, const std::string &defaultValue) const {
        return v.empty()? defaultValue: v;
    }
};

TEST(CommonConfigBaseTest, GetValue_bool) {
    StubConfig stubConfig;

    EXPECT_TRUE(stubConfig.GetValue<bool>("haha", true));
    EXPECT_FALSE(stubConfig.GetValue<bool>("haha", false));

    EXPECT_TRUE(stubConfig.GetValue("haha", true));
    EXPECT_FALSE(stubConfig.GetValue("haha", false));
    stubConfig.v = "0";
    EXPECT_FALSE(stubConfig.GetValue("haha", true));
    EXPECT_FALSE(stubConfig.GetValue<bool>("haha", true));
}

TEST(CommonConfigBaseTest, GetValue_string_01) {
    StubConfig stubConfig;

    EXPECT_TRUE(stubConfig.GetValue("haha", (const char *)nullptr).empty());
    EXPECT_TRUE(stubConfig.GetValue("haha", "").empty());
}

TEST(CommonConfigBaseTest, GetValue_string_02) {
    StubConfig stubConfig;
    stubConfig.v = "C:/Program Files/nvml.dll";

    EXPECT_EQ(stubConfig.GetValue("haha", (const char *)nullptr), stubConfig.v);
    EXPECT_EQ(stubConfig.GetValue("haha", ""), stubConfig.v);
}

TEST(CommonConfigBaseTest, GetValueCompatible) {
    StubConfig stubConfig;

    int val;
    EXPECT_FALSE(stubConfig.GetValue("haha", val, 1));
    EXPECT_EQ(val, 1);
    EXPECT_FALSE(stubConfig.GetValue("haha", val, (int16_t)-1));
    EXPECT_EQ(val, -1);
}
