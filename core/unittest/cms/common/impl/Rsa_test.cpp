//
// Created by 韩呈杰 on 2024/1/10.
//
#include <gtest/gtest.h>
#include "common/Rsa.h"

using namespace common;

TEST(CommonRsaTest, privateDecrypt) {
    std::string deString;
    EXPECT_EQ(-1, Rsa::privateDecrypt({}, {}, deString));
    EXPECT_EQ(-1, Rsa::privateDecrypt("a", {}, deString));
}