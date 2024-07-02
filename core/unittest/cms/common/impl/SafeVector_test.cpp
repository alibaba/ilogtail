//
// Created by 韩呈杰 on 2023/11/7.
//
#include <gtest/gtest.h>
#include "common/SafeVector.h"
#include "common/Logger.h"

TEST(CommonSafeVector, RandomFalse) {
    SafeVector<int> vec;
    EXPECT_TRUE(vec.IsEmpty());
    EXPECT_EQ(vec.Size(), size_t(0));
    bool called = false;
    int index = vec.Random([&](int, int &v) {
        called = true;
    });
    EXPECT_EQ(index, -1);
    EXPECT_FALSE(called);

    EXPECT_FALSE(vec.At(0, [](int v) {}));
}

TEST(CommonSafeVector, Reserve) {
    SafeVector<int> vec;
    vec.Reserve(5);
    EXPECT_TRUE(vec.IsEmpty());
    EXPECT_EQ(vec.Size(), size_t(0));
}

TEST(CommonSafeVector, RandomOK) {
    SafeVector<int> vec;
    vec.Resize(5);
    EXPECT_FALSE(vec.IsEmpty());
    EXPECT_EQ(vec.Size(), size_t(5));
    EXPECT_EQ(vec[0], 0);

    int called = 0;
    int index = vec.Random([&](int, int &v) {
        called++;
    });
    EXPECT_GE(index, 0);
    EXPECT_EQ(called, 1);

    EXPECT_TRUE(vec.At(0, [](int v) {}));
}

TEST(CommonSafeVector, ForEach) {
    SafeVector<int> vec;
    vec.Resize(5, 1);
    EXPECT_FALSE(vec.IsEmpty());
    EXPECT_EQ(vec.Size(), size_t(5));
    EXPECT_EQ(vec[0], 1);

    vec[0] = 0;
    vec[4] = 2;
    EXPECT_EQ(0, vec.At(0));
    EXPECT_EQ(0, const_cast<const SafeVector<int> &>(vec).At(0));
    EXPECT_EQ(0, vec.Front());
    EXPECT_EQ(0, const_cast<const SafeVector<int> &>(vec).Front());
    EXPECT_EQ(2, vec.Back());
    EXPECT_EQ(2, const_cast<const SafeVector<int> &>(vec).Back());

    vec.PushBack(100);
    vec.EmplaceBack(101);
    EXPECT_EQ(size_t(7), vec.Size());
    EXPECT_EQ(100, vec[-2]);
    EXPECT_EQ(101, vec[-1]);

    vec.Clear();
    EXPECT_TRUE(vec.IsEmpty());

    LogInfo("before shrink, vec.capacity() => {}", vec.Capacity());
    vec.ShrinkToFit();
    LogInfo("after  shrink, vec.capacity() => {}", vec.Capacity());
}

TEST(CommonSafeList, PopFront) {
    SafeList<int> lst;
    {
        int v = 0;
        EXPECT_FALSE(lst.PopFront(&v));
    }
    lst.PushBack(1);

    {
        int v = 0;
        EXPECT_TRUE(lst.PopFront(&v));
        EXPECT_EQ(v, 1);
    }

    lst.PushBack(2);
    int val = 0;
    EXPECT_TRUE(PopQueue(lst, val));
    EXPECT_EQ(2, val);
}
