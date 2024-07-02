//
// Created by 韩呈杰 on 2023/4/26.
//
#include <gtest/gtest.h>
#include "common/ModuleData.h"
#include "common/Logger.h"
#include "common/Common.h"  // pid_t

#include <iostream>
#include <cmath>
#include <limits>

using namespace std;
using namespace common;

TEST(ModuleDataTest, convertStringToCollectData) {
    const std::string s = R"(ARGUS_MODULE_DATA disk 8 6 avail 3.14667e+07 free 3.33704e+07 metricValue 0 total 4.11528e+07 use_percent 19.8283 used 7.78244e+06 5 dev_type ZXh0NA== device L2Rldi92ZGEx dir_name Lw== metricName c3lzdGVtLmRpc2s= ns YWNzL2hvc3Q= 1 metricValue 19.8283 3 metricName dm0uRGlza1V0aWxpemF0aW9u mountpoint Lw== ns YWNzL2Vjcw== 5 free 2.51874e+06 metricValue 0 total 2.62144e+06 use_percent 3.91766 used 102699 5 dev_type ZXh0NA== device L2Rldi92ZGEx dir_name Lw== metricName c3lzdGVtLmlub2Rl ns YWNzL2hvc3Q= 1 metricValue 3.91766 3 metricName dm0uRGlza0l1c2VkVXRpbGl6YXRpb24= mountpoint Lw== ns YWNzL2Vjcw== 34 avg_avgqu-sz inf avg_avgrq-sz 67.0667 avg_await 1.6 avg_r_await 1.46667 avg_read_bytes 1.52665e+06 avg_reads 18.6655 avg_svctm 1.33333 avg_util 4.37288 avg_w_await 0.666667 avg_write_bytes 142683 avg_writes 2.0662 max_avgqu-sz inf max_avgrq-sz 349 max_await 9 max_r_await 9 max_read_bytes 1.79241e+07 max_reads 198 max_svctm 9 max_util 43.2 max_w_await 8 max_write_bytes 1.88228e+06 max_writes 18 metricValue 0 min_avgqu-sz 0 min_avgrq-sz 0 min_await 0 min_r_await 0 min_read_bytes 0 min_reads 0 min_svctm 0 min_util 0 min_w_await 0 min_write_bytes 0 min_writes 0 5 dev_type ZXh0NA== device L2Rldi92ZGEx dir_name Lw== metricName c3lzdGVtLmlv ns YWNzL2hvc3Q= 1 metricValue 0 3 diskname L2Rldi92ZGEx metricName dm0uRGlza0lPUmVhZA== ns YWNzL2Vjcw== 1 metricValue 0 3 diskname L2Rldi92ZGEx metricName dm0uRGlza0lPV3JpdGU= ns YWNzL2Vjcw== 1 metricValue 18.9111 3 diskname dmRh metricName c3lzdGVtLmRpc2sudG90YWw= ns YWNzL2hvc3Q=)";

    CollectData collectData;
    ASSERT_TRUE(ModuleData::convertStringToCollectData(s, collectData));

    EXPECT_EQ(collectData.moduleName, "disk");
    EXPECT_EQ(size_t(8), collectData.dataVector.size());
    size_t index = 0;
    for (auto const &metric: collectData.dataVector) {
        std::string metricName;
        auto it = metric.tagMap.find("metricName");
        if (it != metric.tagMap.end()) {
            metricName = it->second;
        }
        LogDebug("[{}] metricName: {}, value count: {}, tag count: {}", index++, metricName,
                      metric.valueMap.size(), metric.tagMap.size());
        EXPECT_NE(metricName, "");
    }
    EXPECT_EQ(collectData.dataVector[4].valueMap["avg_read_bytes"], 1.52665e+06);
}

TEST(ModuleDataTest, getMetricData) {
    const std::string s = R"(2 avg_avgqu-sz inf avg_avgrq-sz 67.0667 0)";

    std::cout << "inf: " << std::numeric_limits<double>::infinity() << std::endl
              << "nan: " << std::numeric_limits<double>::quiet_NaN() << std::endl
              << "snan: " << std::numeric_limits<double>::signaling_NaN() << std::endl;

    std::stringstream ss(s);
    MetricData metricData;
    ModuleData::getMetricData(ss, metricData);

    ASSERT_EQ(size_t(2), metricData.valueMap.size());
    EXPECT_NE(metricData.valueMap.find("avg_avgqu-sz"), metricData.valueMap.end());
    std::cout << "avg_avgqu-sz: " << metricData.valueMap["avg_avgqu-sz"] << std::endl;
    EXPECT_TRUE(std::isinf(metricData.valueMap["avg_avgqu-sz"]));

    EXPECT_EQ(metricData.valueMap["avg_avgrq-sz"], 67.0667);

    EXPECT_EQ(size_t(0), metricData.tagMap.size());

    std::cout << ModuleData::convert(metricData) << std::endl;
    EXPECT_EQ(ModuleData::convert(metricData), s);
}

TEST(ModuleDataTest, formatMetricData_ZeroValue) {
    {
        MetricData data;

        std::string encoded = ModuleData::convert(data);
        std::cout << encoded << std::endl;
        EXPECT_EQ(encoded, "0 0");
    }
    {
        MetricData data;
        data.tagMap["metricName"] = "cpu_total";

        std::string encoded = ModuleData::convert(data);
        std::cout << encoded << std::endl;
        EXPECT_EQ(encoded, "0 1 metricName cpu_total");
    }
}

TEST(ModuleDataTest, formatMetricData) {
    pid_t pids[]{7248, 72480, 2135683, 18128043};
    for (const pid_t &pid: pids) {
        MetricData data;
        data.valueMap["pid"] = pid;

        const std::string expect = fmt::format("1 pid {} 0", pid);
        std::string encoded = ModuleData::convert(data);
        std::cout << "pid: " << (double) pid << ", metric data: " << encoded << std::endl;
        EXPECT_EQ(encoded, expect);

        // pid_t -> double不会损失精度 ，但double序列化为文本才会有精度损失
        EXPECT_EQ((pid_t) ((double) pid), pid);

        std::stringstream ss(encoded);
        MetricData actual;
        ModuleData::getMetricData(ss, actual);
        EXPECT_TRUE(actual.tagMap.empty());
        EXPECT_EQ(size_t(1), actual.valueMap.size());
        EXPECT_EQ((pid_t) actual.valueMap["pid"], pid);
    }
    // 32位范围内int -> double不会损失精度
    // const uint32_t maxUint32 = std::numeric_limits<uint32_t>::max();
    // for (uint32_t i = 0; i < maxUint32; i++) {
    //     double d = i;
    //     if ((uint32_t)((double)(d)) != i) {
    //         std::cout << i << std::endl;
    //     }
    // }

    // 达到上千万后，float无论如何都会有精度损失。
    // const uint32_t maxUint32 = std::numeric_limits<uint32_t>::max();
    // for (uint32_t i = 0; i < maxUint32; i++) {
    //     if ((uint32_t) ((float) (i)) != i) {
    //         std::cout << i << std::endl;
    //     }
    // }
}

TEST(ModuleDataTest, WithoutMetricName) {
    std::cout << "-----------------------------------------------------" << std::endl;
    {
        const char *content = R"(ARGUS_MODULE_DATA process 1 1 MEMORY_RESIDENT_TOTAL 0 0)";
        CollectData data;
        ASSERT_TRUE(ModuleData::convertStringToCollectData(content, data));
        EXPECT_EQ(size_t(1), data.dataVector.size());
    }
    std::cout << "-----------------------------------------------------" << std::endl;
    {
        const char *content = R"(ARGUS_MODULE_DATA process 1 1 MEMORY_RESIDENT_TOTAL 0 0)";
        CollectData data;
        ASSERT_FALSE(ModuleData::convertStringToCollectData(content, data, true));
        EXPECT_EQ(size_t(0), data.dataVector.size());
    }
    std::cout << "-----------------------------------------------------" << std::endl;
    {
        const char *content = R"(ARGUS_MODULE_DATA process 2 1 MEMORY_RESIDENT_TOTAL 0 0)";
        CollectData data;
        ASSERT_FALSE(ModuleData::convertStringToCollectData(content, data));
        EXPECT_EQ(size_t(1), data.dataVector.size());
    }
}

TEST(ModuleDataTest, MetricData_check) {
    MetricData data;
    EXPECT_FALSE(data.check());

    data.tagMap["metricName"] = "cpu";
    EXPECT_FALSE(data.check());

    data.valueMap["metricValue"] = 1;
    EXPECT_FALSE(data.check());

    data.tagMap["ns"] = "acs/host";
    EXPECT_TRUE(data.check());
}


TEST(ModuleDataTest, formatMetricData1) {
    MetricData metricData;
    metricData.valueMap["/dec/:m1"] = 0.2;
    metricData.valueMap["/dec/:m2"] = 0.0;
    metricData.valueMap["/dec/:m3"] = 1.4;
    metricData.tagMap["t1"] = "tag1:test";
    metricData.tagMap["t2"] = "tag2:test";
    metricData.tagMap["t3"] = "tag3:test";
    std::stringstream sStream;
    ModuleData::formatMetricData(metricData, sStream);
    cout << sStream.str() << endl;
    MetricData metricData1;
    ModuleData::getMetricData(sStream, metricData1);
    EXPECT_EQ(size_t(3), metricData1.valueMap.size());
    EXPECT_EQ(size_t(3), metricData1.tagMap.size());
    EXPECT_EQ(1.4, metricData1.valueMap["/dec/:m3"]);
    EXPECT_EQ("tag2:test", metricData1.tagMap["t2"]);
}

TEST(ModuleDataTest, formatMetricData2) {
    MetricData metricData;
    metricData.valueMap["/dec/:m1"] = 0.2;
    metricData.valueMap["/dec/:m2"] = 0.0;
    metricData.valueMap["/dec/:m3"] = 1.4;
    std::stringstream sStream;
    ModuleData::formatMetricData(metricData, sStream);
    cout << sStream.str() << endl;
    MetricData metricData1;
    ModuleData::getMetricData(sStream, metricData1);
    EXPECT_EQ(size_t(3), metricData1.valueMap.size());
    EXPECT_EQ(size_t(0), metricData1.tagMap.size());
    EXPECT_EQ(1.4, metricData1.valueMap["/dec/:m3"]);
}

TEST(ModuleDataTest, formatMetricData3) {
    MetricData metricData;
    std::stringstream sStream;
    ModuleData::formatMetricData(metricData, sStream);
    cout << sStream.str() << endl;
    MetricData metricData1;
    ModuleData::getMetricData(sStream, metricData1);
    EXPECT_EQ(size_t(0), metricData1.valueMap.size());
    EXPECT_EQ(size_t(0), metricData1.tagMap.size());
}

TEST(ModuleDataTest, convertCollectDataToString1) {
    CollectData collectData;
    collectData.moduleName = "testModuleName";
    for (int i = 0; i < 10; i++) {
        std::stringstream stri;
        stri << i;
        string ss = stri.str();
        MetricData metricData;
        metricData.valueMap["/dec/:m1" + ss] = 0.2 + i;
        metricData.valueMap["/dec/:m2" + ss] = 0.0 + i;
        metricData.valueMap["/dec/:m3" + ss] = 1.4 + i;
        metricData.tagMap["t1" + ss] = "tag1:test" + ss;
        metricData.tagMap["t2" + ss] = "tag2:test" + ss;
        metricData.tagMap["t3" + ss] = "tag3:test" + ss;
        metricData.tagMap["metricName"] = fmt::format("test-{}", i);
        metricData.valueMap["metricValue"] = 0;
        metricData.tagMap["ns"] = "test";
        collectData.dataVector.push_back(metricData);
    }
    string content;
    ModuleData::convertCollectDataToString(collectData, content);
    cout << content << endl;
    CollectData collectData1;
    EXPECT_TRUE(ModuleData::convertStringToCollectData(content, collectData1));
    EXPECT_EQ(collectData.moduleName, collectData1.moduleName);
    EXPECT_EQ(size_t(10), collectData1.dataVector.size());
    for (size_t i = 0; i < collectData1.dataVector.size(); i++) {
        std::stringstream stri;
        stri << i;
        string ss = stri.str();
        MetricData metricData = collectData1.dataVector[i];
        EXPECT_EQ(metricData.valueMap["/dec/:m1" + ss], 0.2 + i);
        EXPECT_EQ(metricData.valueMap["/dec/:m2" + ss], 0.0 + i);
        EXPECT_EQ(metricData.valueMap["/dec/:m3" + ss], 1.4 + i);
        EXPECT_EQ(metricData.tagMap["t1" + ss], "tag1:test" + ss);
        EXPECT_EQ(metricData.tagMap["t2" + ss], "tag2:test" + ss);
        EXPECT_EQ(metricData.tagMap["t3" + ss], "tag3:test" + ss);
        EXPECT_EQ(fmt::format("test-{}", i), metricData.tagMap["metricName"]);
        EXPECT_EQ(0, metricData.valueMap["metricValue"]);
        EXPECT_EQ("test", metricData.tagMap["ns"]);
    }
}

TEST(ModuleDataTest, convertCollectDataToString2) {
    CollectData collectData;
    collectData.moduleName = "testModuleName";
    string content;
    ModuleData::convertCollectDataToString(collectData, content);
    cout << content << endl;
    CollectData collectData1;
    ModuleData::convertStringToCollectData(content, collectData1);
    EXPECT_EQ(collectData.moduleName, collectData1.moduleName);
    EXPECT_EQ(size_t(0), collectData1.dataVector.size());
}
