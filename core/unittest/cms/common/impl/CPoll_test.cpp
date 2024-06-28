//
// Created by 韩呈杰 on 2024/1/15.
//
#include <gtest/gtest.h>
#include "common/CPoll.h"
#include "common/PollEventBase.h"
#include "common/Logger.h"

using namespace common;

void StartGlobalPoll() {
    SingletonPoll::Instance()->runIt();
}

struct MockPollEvent: PollEventBase {
    std::shared_ptr<NetWorker> net;

    MockPollEvent(const std::shared_ptr<NetWorker> &r): net(r){
    }
    int readEvent(const std::shared_ptr<PollEventBase> &myself) override {
        return 0;
    }
};

TEST(CommonPollTest, Destruct) {
    Poll poll; // 无异常
    EXPECT_EQ(-1, poll.add(std::shared_ptr<PollEventBase>(), std::shared_ptr<NetWorker>()));

    auto net = std::make_shared<NetWorker>(__FUNCTION__);
    ASSERT_EQ(0, net->connect("www.aliyun.com", 443));

    auto  pollEvent = std::make_shared<MockPollEvent>(std::ref(net));
    EXPECT_EQ(0, poll.add(pollEvent, net));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    EXPECT_EQ(-1, poll.add(pollEvent, net));  // sock already exist
    // 确保走到了正确的错误分支上
    auto cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("socket already in pollset"));
}
