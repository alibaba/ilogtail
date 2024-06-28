//
// Created by 韩呈杰 on 2024/1/15.
//
#include <gtest/gtest.h>
#include "core/ChannelManager.h"
#include "common/ScopeGuard.h"

using namespace argus;
using namespace common;

struct MockBaseOutputChannel : OutputChannel {
    int messageCount = 0;
    int addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                    int exitCode, const std::string &result, const std::string &conf, bool reportStatus,
                    const std::string &mid) override {
        messageCount++;
		return 0;
    }
};

TEST(CoreChannelManagerTest, SendMsgToNextModule) {
    ChannelManager cm;
    std::string chanName = __FUNCTION__;

    std::vector<std::pair<std::string, std::string>> outputs{{chanName, ""}};
    EXPECT_EQ(0, cm.sendMsgToNextModule("", outputs, std::chrono::system_clock::now(), 0, "", true, ""));

    cm.AddOutputChannel(chanName, std::make_shared<MockBaseOutputChannel>());

    EXPECT_EQ(1, cm.sendMsgToNextModule("", outputs, std::chrono::system_clock::now(), 0, "", true, ""));
    EXPECT_EQ(1, cm.Get<MockBaseOutputChannel>(chanName)->messageCount);

    cm.End();
}

TEST(CoreChannelManagerTest, SendMetricToNextModule) {
    ChannelManager cm;

    std::string chanName = __FUNCTION__;
    std::vector<std::pair<std::string, std::string>> outputs{{chanName, ""}};
    std::vector<CommonMetric> vecMetrics;
    EXPECT_EQ(0, cm.SendMetricsToNextModule(vecMetrics, outputs));


    struct MockOutputChannel : MockBaseOutputChannel {
        int count = 0;

        int AddCommonMetrics(const std::vector<common::CommonMetric> &metrics, std::string conf) override {
            count++;
			return 0;
        }
    };
    cm.AddOutputChannel(chanName, std::make_shared<MockOutputChannel>());

    EXPECT_EQ(1, cm.SendMetricsToNextModule(vecMetrics, outputs));
    EXPECT_EQ(1, cm.Get<MockOutputChannel>(chanName)->count);
}

TEST(CoreChannelManagerTest, SendResultMapToNextModule) {
    ChannelManager cm;
    std::string chanName = __FUNCTION__;

    std::vector<std::unordered_map<std::string, std::string>> resultMap;
    const std::vector<std::pair<std::string, std::string>> outputs{{chanName, ""}};
    EXPECT_EQ(0, cm.SendResultMapToNextModule(resultMap, outputs)); // unknown channel

    struct MockOutputChannel : MockBaseOutputChannel {
        int count = 0;

        int AddResultMap(const std::vector<std::unordered_map<std::string, std::string>> &resultMaps,
                          std::string conf) override {
            count++;
            return 0;
        }
    };
    cm.AddOutputChannel(chanName, std::make_shared<MockOutputChannel>());
    EXPECT_EQ(1, cm.SendResultMapToNextModule(resultMap, outputs));
    EXPECT_EQ(1, cm.Get<MockOutputChannel>(chanName)->count);
}

TEST(CoreChannelManagerTest, SendRawDataToNextModule) {
    ChannelManager cm;
    std::string chanName = __FUNCTION__;

    RawData rawData;
    const std::vector<std::pair<std::string, std::string>> outputs{{chanName, ""}};
    EXPECT_EQ(0, cm.SendRawDataToNextModule(rawData, outputs)); // unknown channel

    struct MockOutputChannel : MockBaseOutputChannel {
        int count = 0;

        int AddRawData(const common::RawData &rawData, std::string conf) override {
            count++;
			return 0;
        }
    };
    cm.AddOutputChannel(chanName, std::make_shared<MockOutputChannel>());
    EXPECT_EQ(1, cm.SendRawDataToNextModule(rawData, outputs));
    EXPECT_EQ(1, cm.Get<MockOutputChannel>(chanName)->count);
}
