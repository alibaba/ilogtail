//
// Created by 韩呈杰 on 2023/6/15.
//
#include <gtest/gtest.h>
#include "common/SafeMap.h"

#include <thread>

TEST(CommonSafeMapTest, StdMap) {
    SafeMap<int, int> data;
    // SafeMap<int, int> b(data); // SafeMap不支持拷贝、赋值、Move语义
    EXPECT_TRUE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(0));
    std::cout << "data.size() = " << data.Size() << std::endl;

    data.Emplace(1, 2);
    EXPECT_FALSE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(1));
    EXPECT_EQ(data[1], 2);
    EXPECT_TRUE(data.Contains(1));
    EXPECT_FALSE(data.Contains(-1));

    EXPECT_FALSE(data.MustSet(10, 20));
    EXPECT_FALSE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(2));
    EXPECT_EQ(data[10], 20);

    data.Erase(10);
    {
        int v;
        EXPECT_FALSE(data.Find(10, v));
    }

    data.Clear();
    EXPECT_TRUE(data.IsEmpty());
}

TEST(CommonSafeMapTest, UnorderedMap) {
    SafeUnorderedMap<int, int> data;
    EXPECT_TRUE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(0));
    std::cout << "data.size() = " << data.Size() << std::endl;

    data.Emplace(1, 2);
    data.Emplace(1, 3);
    int target = 0;
    EXPECT_TRUE(data.IfExist(1, [&](const int &v) { target = v; }));
    EXPECT_EQ(target, 2);
    EXPECT_FALSE(data.IfExist(3, [&](const int &v) { }));
    EXPECT_FALSE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(1));
    EXPECT_EQ(data[1], 2);

    EXPECT_FALSE(data.MustSet(10, 20));
    EXPECT_FALSE(data.IsEmpty());
    EXPECT_EQ(data.Size(), size_t(2));
    EXPECT_EQ(data[10], 20);

    data.Erase(10);
    {
        int v;
        EXPECT_FALSE(data.Find(10, v));
    }

    data.Clear();
    EXPECT_TRUE(data.IsEmpty());

    std::cout << "start to concurrent writing" << std::endl;
    // 并发赋值(需要并发控制，所以性能会很差)
    volatile int loopCount = 100 * 1000;
    auto loopAssign = [&](int k, int v){
        for (int i = 0; i < loopCount; i++) {
            data.Set(k, v);
            data.MustSet(v, k);
        }
    };

    std::thread thread1(loopAssign, 1, 2);
    std::thread thread2(loopAssign, 2, 3);
    thread1.join();
    thread2.join();
    std::cout << "end of concurrent writing" << std::endl;

    Sync(data) {
        for (const auto &it: data) {
            std::cout << "[" << it.first << "] = " << it.second << std::endl;
        }
    }}}
    EXPECT_EQ(data.Size(), size_t(3));
    EXPECT_EQ(data[1], 2);
    EXPECT_EQ(data[3], 2);
    EXPECT_TRUE(data[2] == 1 || data[2] == 3);


    int v = 0;
    EXPECT_TRUE(data.Find(3, v));
    EXPECT_EQ(v, 2);
    EXPECT_FALSE(data.Find(65535, v));
}

TEST(CommonSafeMapTest, SetIfAbsent) {
    SafeMap<int, int> data;
    EXPECT_FALSE(data.SetIfExist(1, 2));
    EXPECT_TRUE(data.IsEmpty());
    EXPECT_EQ(2, data.ComputeIfAbsent(1, [](){return 2;}));
    EXPECT_EQ(2, data.SetIfAbsent(1, 3));
    EXPECT_EQ(size_t(1), data.GetCopy().size());
}

TEST(CommonSafeMapTest, SetIfExist) {
    SafeMap<int, int> data;
    EXPECT_EQ(2, data.ComputeIfAbsent(1, [](){return 2;}));
    EXPECT_TRUE(data.SetIfExist(1, 3));
    EXPECT_EQ(3, data.SetIfAbsent(1, 4));
    EXPECT_EQ(3, data[1]);
}

TEST(CommonSafeMapTest, ThreadSafeTest) {
    SafeMap<int, int> data;

    std::cout << "start to concurrent write" << std::endl;
    // 并发赋值(需要并发控制，所以性能会很差), 运行时长大概500ms
    volatile int loopCount = 100 * 1000;
    auto loopAssign = [&](int k, int v){
        for (int i = 0; i < loopCount; i++) {
            data.MustSet(k, v);
            data.MustSet(v, k);
        }
    };

    std::thread thread1(loopAssign, 1, 2);
    std::thread thread2(loopAssign, 2, 3);
    thread1.join();
    thread2.join();
    std::cout << "end of concurrent writing" << std::endl;;

    size_t size = SyncCall(data, { return data.size(); });
    EXPECT_EQ(data.Get().size(), size);
    EXPECT_TRUE(data.Contains(1));
    Sync(data) {
        for (const auto &it: data) {
            std::cout << "[" << it.first << "] = " << it.second << std::endl;
        }
    }}}
    EXPECT_EQ(data.Size(), size_t(3));
    EXPECT_EQ(data[1], 2);
    EXPECT_EQ(data[3], 2);
    EXPECT_TRUE(data[2] == 1 || data[2] == 3);


    int v = 0;
    EXPECT_TRUE(data.Find(3, v));
    EXPECT_EQ(v, 2);
    EXPECT_FALSE(data.Find(65535, v));
}

TEST(CommonSafeMapTest, Set) {
    SafeMap<int, int> data;
    EXPECT_TRUE(SyncCall(data, {return data.empty();}));
    std::map<int, int> data2 {
            {1, 1},
            {2, 2},
    };

    data = data2;
    EXPECT_EQ(size_t(2), SyncCall(data, {return data.size();}));
}
