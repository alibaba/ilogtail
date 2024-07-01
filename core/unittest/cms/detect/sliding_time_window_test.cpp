#include "detect/sliding_time_window.h"

#include <gtest/gtest.h>

#include <thread>
#include <chrono>

#include "common/Config.h"
#include "common/Logger.h"

using namespace std;
using namespace common;

class Detect_SlidingTimeWindowTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
        pShared = new SlidingTimeWindow<int>(std::chrono::seconds{1});
    }

    void TearDown() override {
        delete pShared;
        pShared = nullptr;
    }

    SlidingTimeWindow<int> *pShared = nullptr;
};

TEST_F(Detect_SlidingTimeWindowTest, Test1) {
    pShared->Update(10);
	std::this_thread::sleep_for(std::chrono::microseconds{ 1000 * 1000 + 10 });
    pShared->RemoveTimeoutValue();
    EXPECT_EQ(pShared->GetAvgValue(), 0.0);
    EXPECT_EQ(pShared->GetMaxValue(), 0);
    EXPECT_EQ(pShared->GetMinValue(), 0);
}

TEST_F(Detect_SlidingTimeWindowTest, Test2) {
    pShared->Update(10);
	std::this_thread::sleep_for(std::chrono::microseconds{ 1000 * 1000 + 10 });
	pShared->Update(1);
    pShared->Update(2);
    pShared->RemoveTimeoutValue();
    EXPECT_EQ(pShared->GetAvgValue(), 1.5);
    EXPECT_EQ(pShared->GetMaxValue(), 2);
    EXPECT_EQ(pShared->GetMinValue(), 1);
}

TEST_F(Detect_SlidingTimeWindowTest, Test3) {
    pShared->Update(100);
	std::this_thread::sleep_for(std::chrono::microseconds{ 1000 * 1000 + 10 });
	for (int i = 1; i < 50; i++) {
        pShared->Update(i);
    }
    pShared->RemoveTimeoutValue();
    EXPECT_EQ(pShared->GetAvgValue(), 25.0);
    EXPECT_EQ(pShared->GetMaxValue(), 49);
    EXPECT_EQ(pShared->GetMinValue(), 1);
}