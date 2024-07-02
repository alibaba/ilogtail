//
// Created by 韩呈杰 on 2023/2/6.
//
#include <gtest/gtest.h>

#include "cloudMonitor/metric_calculate_2.h"
#include "cloudMonitor/tcp_ext_collect.h"

using namespace cloudMonitor;

TEST(Cms_MetricCalculate2Test, Normal) {
    MetricCalculate2<TcpExtMetric, double> obj{};
    {
        TcpExtMetric last;
        EXPECT_FALSE(obj.GetLastValue(last));
        EXPECT_FALSE((bool)obj.GetLastValue());
        EXPECT_EQ(decltype(obj.Count())(0), obj.Count());
    }

    TcpExtMetric metric;
    metric.delayedACKs = 1.0;
    obj.AddValue(metric);
    EXPECT_EQ(size_t(1), obj.Count());
    {
        TcpExtMetric last;
        EXPECT_TRUE(obj.GetLastValue(last));
        EXPECT_TRUE((bool)obj.GetLastValue());

        TcpExtMetric max;
        TcpExtMetric min;
        TcpExtMetric avg;
        EXPECT_TRUE(obj.Stat(max, min, avg));
        EXPECT_EQ(1.0, max.delayedACKs);
        EXPECT_EQ(1.0, min.delayedACKs);
        EXPECT_EQ(1.0, avg.delayedACKs);
    }

    TcpExtMetric metric2;
    metric2.delayedACKs = 3.0;
    obj.AddValue(metric2);
    EXPECT_EQ(size_t(2), obj.Count());
    {
        TcpExtMetric last;
        EXPECT_TRUE(obj.GetLastValue(last));
        EXPECT_EQ(3.0, last.delayedACKs);

        TcpExtMetric max;
        EXPECT_TRUE(obj.GetMaxValue(max));
        EXPECT_EQ(3.0, max.delayedACKs);

        TcpExtMetric min;
        EXPECT_TRUE(obj.GetMinValue(min));
        EXPECT_EQ(1.0, min.delayedACKs);

        TcpExtMetric avg;
        EXPECT_TRUE(obj.GetAvgValue(avg));
        EXPECT_EQ(2.0, avg.delayedACKs);
    }

    EXPECT_EQ(size_t(2), obj.Count());
    obj.Reset();
    EXPECT_EQ(size_t(0), obj.Count());
}
