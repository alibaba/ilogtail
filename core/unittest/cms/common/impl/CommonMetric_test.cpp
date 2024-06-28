//
// Created by 韩呈杰 on 2023/4/19.
//
#include <gtest/gtest.h>
#include "common/CommonMetric.h"

TEST(CommonCommonMetricTest, toString) {
    common::CommonMetric metric;
    metric.name = "cpu";
    metric.value = 1;
    metric.timestamp = 1681875279;
    metric.tagMap["cpu_id"] = "1\n2";

    std::string s = metric.toString();
    std::cout << s << std::endl;
    EXPECT_EQ(s, R"XX(cpu{cpu_id="1\n2"} 1 1681875279)XX");
}
