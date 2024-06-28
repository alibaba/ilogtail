#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include "common/Base16.h"

#include "common/Logger.h"
#include "common/StringUtils.h"

using namespace std;
using namespace common;

class CommonBase16Test : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(CommonBase16Test, test) {
    // windows使用unicode，而linux使用utf8这两者导致在某些汉字处理上有差异
    const string str = "~12%^&*(/event/custom/upload";
    string base16 = "7E3132255E262A282F6576656E742F637573746F6D2F75706C6F6164";
    string result = Base16::Encode((unsigned char *) str.c_str(), str.size(), true);
    EXPECT_EQ(result, base16);
    result = Base16::Decode(result.c_str());
    EXPECT_EQ(result, str);
    result = Base16::Encode((unsigned char *) str.c_str(), str.size(), false);
    cout << result << endl;
    EXPECT_EQ(StringUtils::ToLower(result), StringUtils::ToLower(base16));
}
