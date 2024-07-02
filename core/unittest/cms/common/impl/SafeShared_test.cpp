//
// Created by 韩呈杰 on 2023/11/6.
//
#include <gtest/gtest.h>
#include <list>
#include "common/SafeShared.h"

TEST(CommonSyncSharedTest, String_01) {
    SafeShared<std::string> data;
    EXPECT_TRUE(SyncCall(data, {return data->empty();}));
    EXPECT_TRUE(data.IsEmpty());
}

TEST(CommonSyncSharedTest, String_02) {
    SafeShared<std::string> data("hello");
    const std::string &actual = SyncCall(data, { return *data; });
    EXPECT_EQ(actual, "hello");
    EXPECT_FALSE(data.IsEmpty());
}

TEST(CommonSyncSharedTest, Map) {
    SafeSharedMap<int, int> data;
    EXPECT_TRUE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(0));

    data.Emplace(1, 1);
    EXPECT_EQ(data.Values().size(), size_t(1));
    EXPECT_TRUE(data.Contains(1));
    EXPECT_FALSE(data.IsEmpty());

    std::map<int, int> v;
    EXPECT_TRUE(data.Find(1, v));
    EXPECT_EQ(size_t(1), v.size());
}

TEST(CommonSyncSharedTest, UnorderedMap) {
    SafeSharedUnorderedMap<int, int> data;
    EXPECT_TRUE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(0));

    data.Emplace(1, 1);
    EXPECT_EQ(data.Values().size(), size_t(1));
    EXPECT_TRUE(data.Contains(1));
    EXPECT_FALSE(data.IsEmpty());
}

TEST(CommonSyncSharedTest, List) {
    SafeShared<std::list<int>> data;
    EXPECT_TRUE(data.IsEmpty());

    data.PushBack(1);
    EXPECT_EQ(size_t(1), data.Size());
    EXPECT_FALSE(data.IsEmpty());

    data.EmplaceBack(2);
    EXPECT_EQ(size_t(2), data.Size());
    EXPECT_FALSE(data.IsEmpty());

    int v = SyncCall(data, { return *data->rbegin(); });
    EXPECT_EQ(2, v);
}
