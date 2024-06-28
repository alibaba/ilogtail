//
// Created by 韩呈杰 on 2023/5/30.
//
#include <gtest/gtest.h>
#include "core/OutputChannel.h"

using namespace argus;

// 确保以下缺省实现无异常
TEST(Core_OutputChannelTest, EmptyVirtualFunction) {
    struct MockOutputChannel : OutputChannel {
        int addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                        int exitCode, const std::string &result, const std::string &conf, bool reportStatus,
                        const std::string &mid) override {
			return 0;
        }
    };
    {
        MockOutputChannel chan;

        EXPECT_EQ(0, chan.AddCommonMetric(common::CommonMetric{}, {}));
        EXPECT_EQ(0, chan.AddCommonMetrics(std::vector<common::CommonMetric>{}, {}));
        EXPECT_EQ(0, chan.AddResultMap(std::vector<std::unordered_map<std::string, std::string>>{}, {}));
        EXPECT_EQ(0, chan.AddRawData(common::RawData{}, {}));

        common::CommonMetric metric;
        chan.GetChannelStatus(metric);

        EXPECT_TRUE(chan.runIt());

        // ~chan
    }
}

TEST(Core_OutputChannelTest, ToPayloadString) {
    EXPECT_EQ("7289.23", OutputChannel::ToPayloadString(7289.234));  // 常规(只输出两位小数)
    EXPECT_EQ("7289.2", OutputChannel::ToPayloadString(7289.2));     // 常规
    EXPECT_EQ("7289", OutputChannel::ToPayloadString(7289.0));       // 常规
    EXPECT_EQ("3588972", OutputChannel::ToPayloadString(3588972));   // 百万
    EXPECT_EQ("35889721", OutputChannel::ToPayloadString(35889721)); // 千万(不会因为精度问题导致整数部分受损)
}