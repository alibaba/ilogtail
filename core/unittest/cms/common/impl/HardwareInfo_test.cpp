//
// Created by 韩呈杰 on 2023/7/1.
//
#include <gtest/gtest.h>
#include "common/HardwareInfo.h"
#include "common/Logger.h"

using namespace common;
TEST(Common_HardwareInfoTest, toString) {
    HardwareInfo hwInfo;
    hwInfo.display();
    LogInfo("{}", hwInfo.toString());
}
