//
// Created by 韩呈杰 on 2023/10/25.
//
#include <gtest/gtest.h>
#include "common/ModuleTool.hpp"

using namespace common;

class FakeModule {
    std::string s = "hello";
public:
    int Init() {
        return 0;
    }

    int Collect(std::string &d)  {
        d = s;
        return static_cast<int>(s.size());
    }
};


IMPLEMENT_MODULE(fake) {
    return module::NewHandler<FakeModule>();
}

TEST(CommonFakeModuleTest, collect) {
    IHandler *pModule = fake_init(nullptr);

    char *buf = nullptr;
    EXPECT_EQ(5, fake_collect(pModule, &buf));

    EXPECT_EQ(5, fake_collect(pModule, &buf));
    EXPECT_STREQ(buf, "hello");

    fake_exit(pModule);
}
