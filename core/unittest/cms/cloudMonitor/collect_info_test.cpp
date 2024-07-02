//
// Created by 韩呈杰 on 2023/8/8.
//
#include <gtest/gtest.h>
#include "cloudMonitor/collect_info.h"

TEST(CmsCollectInfoTest, Version) {
    std::cout << "Version : " << cloudMonitor::CollectInfo::GetVersion() << std::endl;
    EXPECT_FALSE(cloudMonitor::CollectInfo::GetVersion().empty());
}

TEST(CmsCollectInfoTest, CompileTime) {
    std::cout << "Compile Time: " << cloudMonitor::CollectInfo::GetCompileTime() << std::endl;
    EXPECT_FALSE(cloudMonitor::CollectInfo::GetCompileTime().empty());
}

TEST(CmsCollectInfoTest, GetSet) {
    cloudMonitor::CollectInfo ci;
    EXPECT_EQ(double(0), ci.GetLastCommitCost());
    ci.SetLastCommitCost(1);
    EXPECT_EQ(1.0, ci.GetLastCommitCost());

    EXPECT_EQ(0, ci.GetLastCommitCode());
    ci.SetLastCommitCode(1);
    EXPECT_EQ(1, ci.GetLastCommitCode());

    EXPECT_EQ("", ci.GetLastCommitMsg());
    ci.SetLastCommitMsg("1");
    EXPECT_EQ("1", ci.GetLastCommitMsg());

    EXPECT_EQ(size_t(0), ci.GetPutMetricFailCount());
    ci.SetPutMetricFailCount(1);
    EXPECT_EQ(size_t(1), ci.GetPutMetricFailCount());

    EXPECT_EQ(double(0), ci.GetPutMetricFailPerMinute());
    ci.SetPutMetricFailPerMinute(1);
    EXPECT_EQ(double(1), ci.GetPutMetricFailPerMinute());

    EXPECT_EQ(size_t(0), ci.GetPutMetricSuccCount());
    ci.SetPutMetricSuccCount(1);
    EXPECT_EQ(size_t(1), ci.GetPutMetricSuccCount());

    EXPECT_EQ(size_t(0), ci.GetPullConfigFailCount());
    ci.SetPullConfigFailCount(1);
    EXPECT_EQ(size_t(1), ci.GetPullConfigFailCount());

    EXPECT_EQ(double(0), ci.GetPullConfigFailPerMinute());
    ci.SetPullConfigFailPerMinute(1);
    EXPECT_EQ(double(1), ci.GetPullConfigFailPerMinute());

    EXPECT_EQ(size_t(0), ci.GetPullConfigSuccCount());
    ci.SetPullConfigSuccCount(1);
    EXPECT_EQ(size_t(1), ci.GetPullConfigSuccCount());
}
