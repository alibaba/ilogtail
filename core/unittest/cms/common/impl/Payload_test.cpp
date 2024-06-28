//
// Created by 韩呈杰 on 2023/5/30.
//
#include <gtest/gtest.h>
#include "common/Payload.h"

TEST(CmsPayLoadTest, ToPayloadString) {
    EXPECT_EQ("7289.23", ToPayloadString(7289.234));  // 常规(只输出两位小数)
    EXPECT_EQ("7289.2", ToPayloadString(7289.2));     // 常规
    EXPECT_EQ("7289", ToPayloadString(7289.0));       // 常规
    EXPECT_EQ("3588972", ToPayloadString(3588972));   // 百万
    EXPECT_EQ("35889721", ToPayloadString(35889721)); // 千万(不会因为精度问题导致整数部分受损)
}
