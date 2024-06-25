//
// Created by 韩呈杰 on 2024/1/17.
//
#include <gtest/gtest.h>
#include "core/argus_manager.h"
#include "core/TaskManager.h"

using namespace common;
using namespace argus;

TEST(CoreArgusManagerTest, GetOutputChannel) {
    ArgusManager argusManager;
    EXPECT_FALSE((bool) argusManager.GetOutputChannel("~!@#$"));
}

TEST(CoreArgusManagerTest, GetStatus) {
#define metricName "FakeChannel"
    struct FakeChannel : OutputChannel {
        int addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                        int exitCode, const std::string &result, const std::string &conf, bool reportStatus,
                        const std::string &mid) override {
			return 0;
        }
        void GetChannelStatus(common::CommonMetric &metric) override {
            metric.name = metricName;
        }
    };

    ArgusManager argusManager;
    argusManager.getChannelManager()->mChannelMap.MustSet("alimonitor", std::make_shared<FakeChannel>());
    EXPECT_TRUE((bool)argusManager.getChannelManager()->Get<FakeChannel>("alimonitor"));

    std::vector<CommonMetric> metrics;
    argusManager.GetStatus(metrics);
    const CommonMetric *target = nullptr;
    for (const CommonMetric &m: metrics) {
        if (m.name == metricName) {
            target = &m;
            break;
        }
    }
    EXPECT_NE(nullptr, target);
#undef metricName
}