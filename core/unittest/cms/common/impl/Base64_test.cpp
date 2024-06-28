#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include "common/Base64.h"

using namespace std;
using namespace common;

class Base64Test : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(Base64Test, test) {
    string src = "{{{{///}}}dddddddte注册表是为Windows NT和Windows95中所有32位硬件/驱动和32位应用程序设计的数据文件，用于存储系统和应用程序的设置信息。16位驱动在Winnt (Windows New Technology)下无法工作，所以所有设备都通过注册表来控制，一般这些是通过BIOS（基本输入输出系统）来控制的";
    string encode = Base64::UrlEncode((const unsigned char *) src.c_str(), src.size());
    cout << encode << endl;
    EXPECT_EQ(Base64::UrlDecode(encode), src);
}
