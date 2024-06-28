//
// Created by 韩呈杰 on 2023/1/12.
//
#include <gtest/gtest.h>
#include "common/ScopeGuard.h"

#include <string>

TEST(CommonScopeGuardTest, Guard) {
    std::string s;
    {
        // 确保defer是在代码块退出时再执行，同时是按由下向上的顺序
        defer(s.push_back('3'));
        s.push_back('0');
        defer(s.push_back('2'));
        s.push_back('1');
    }
    EXPECT_EQ(s, "0123");
}

TEST(CommonScopeGuardTest, Delete) {
    int count = 0;

    struct A {
        int &_count;

        explicit A(int &n) : _count(n) {}

        ~A() { ++_count; }
    };

    A *p = new A(count);
    Delete(p);
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(1, count);
}
