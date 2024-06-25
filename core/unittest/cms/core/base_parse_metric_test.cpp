//
// Created by 韩呈杰 on 2023/3/27.
//
#include <gtest/gtest.h>
#include "core/base_parse_metric.h"

#include <utility>
#include <vector>

#include "core/TaskManager.h"

using namespace argus;

struct FakeLabelGetter : argus::LabelGetter {
    const std::string value;

    std::string get(const std::string &key) const override {
        return value;
    }

    FakeLabelGetter(std::string v) : value(std::move(v)) {}
};

TEST(CoreBaseParseMetricTest, ParseAddLabelInfo) {
    LabelAddInfo item;
    item.type = 0;
    item.name = "__cluster__";
    std::vector<LabelAddInfo> labelAndInfos{&item, &item + 1};

    {
        FakeLabelGetter getter("cn-hangzhou");
        std::map<std::string, std::string> addTagMap;
        EXPECT_TRUE(BaseParseMetric::ParseAddLabelInfo(labelAndInfos, addTagMap, nullptr, &getter));
        EXPECT_EQ(addTagMap[item.name], "cn-hangzhou");
    }
    {
        std::map<std::string, std::string> addTagMap;
        FakeLabelGetter getter{""};
        EXPECT_FALSE(BaseParseMetric::ParseAddLabelInfo(labelAndInfos, addTagMap, nullptr, &getter));
    }
}

TEST(CoreBaseParseMetricTest, AddMetric) {
    BaseParseMetric metric({}, {});
    EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, metric.AddMetric(""));
}
