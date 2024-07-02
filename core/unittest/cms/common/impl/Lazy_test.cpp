//
// Created by 韩呈杰 on 2023/12/4.
//
#include <gtest/gtest.h>
#include "common/Lazy.h"
#include "common/Nop.h"

TEST(CommonLazyTest, DoTest) {
    {
        Lazy<int> lazyInt;
        EXPECT_EQ(nullptr, lazyInt.instance);
        EXPECT_EQ(0, *lazyInt.Instance());
    }
    {
        Lazy<std::string> lazyString("hello");
        EXPECT_EQ(size_t(5), lazyString->size());
    }

    struct tagStub {
        int * const counter;

        tagStub(int *p): counter(p) {
            if (counter) {
                ++*counter;
            }
        }
        ~tagStub() {
            if (counter) {
                ++*counter;
            }
        }
    };
    {
        int c = 0;
        {
            Lazy<tagStub> lazy(&c);
            EXPECT_EQ(nullptr, lazy.instance);
            EXPECT_EQ(1, *lazy.Instance()->counter);
            EXPECT_EQ(1, c);
        }
        EXPECT_EQ(2, c);
    }

    {
        int c = 0;
        tagStub *p;
        {
            Lazy<tagStub, NopDeleter<tagStub>> lazy(&c);
            EXPECT_EQ(nullptr, lazy.instance);
            EXPECT_EQ(1, *lazy.Instance()->counter);
            EXPECT_EQ(1, c);
            p = lazy.Instance();
        }
        EXPECT_EQ(1, c);
        delete p;
    }
}
