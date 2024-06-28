//
// Created by 韩呈杰 on 2023/4/3.
//
#include <gtest/gtest.h>

#include "common/Uuid.h"

TEST(CommonSystemInformationCollectorTest, NewUuid) {
    std::string uuid1 = NewUuid();
    EXPECT_NE(uuid1, "");
    std::string uuid2 = NewUuid();
    EXPECT_NE(uuid2, "");

    EXPECT_NE(uuid1, uuid2);

    std::cout << "[1] " << uuid1 << std::endl
              << "[2] " << uuid2 << std::endl;
}
