//
// Created by 韩呈杰 on 2023/3/2.
//
#include <gtest/gtest.h>
#include "core/TaskManager.h"
#include "core/argus_manager.h"
#include "core/ModuleScheduler2.h"

#include <memory>

#include "common/JsonValue.h"
#include "common/StringUtils.h"

using namespace argus;
using namespace common;

TEST(Core_TaskManagerTest, NodeItem) {
    argus::NodeItem nodeItem;
    nodeItem.instanceId = "i-123";
    std::cout << nodeItem.str(true) << std::endl;
    EXPECT_NE(nodeItem.str(true), "");
}

TEST(Core_TaskManagerTest, ModuleItem) {
    TaskManager tm;
    ArgusManager am(&tm);
    auto *old = SingletonArgusManager::swap(&am);
    defer(SingletonArgusManager::swap(old));

    {
        // TaskManager tm;
        tm.SetModuleItems(std::make_shared<std::map<std::string, ModuleItem>>());
        EXPECT_TRUE((bool) am.getModuleScheduler()->mItems);
        EXPECT_TRUE((bool) tm.ModuleItems());
    }

    am.swap(std::shared_ptr<ModuleScheduler2>(), &tm);
    {
        // TaskManager tm;
        tm.SetModuleItems(std::make_shared<std::map<std::string, ModuleItem>>());
        EXPECT_FALSE((bool) tm.ModuleItems());
    }
}

TEST(Core_TaskManagerTest, tagCron) {
    tagCron cron;
    cron = "[* * * * * ? 2010]";
    EXPECT_FALSE(cron.empty());

    cron = "";
    EXPECT_EQ(cron.string(), "");
    EXPECT_TRUE(cron.empty());

    cron = QUARTZ_ALL;
    EXPECT_EQ(cron.string(), QUARTZ_ALL);
    EXPECT_TRUE(cron.empty());
}

TEST(Core_TaskManagerTest, tagKeyValuesIsDataValid01) {
    const char *szJson = R"([{
"label_name": "hello",
"label_values": ["*"]
}])";
    std::string errMsg;
    json::Array arr = json::parseArray(szJson, errMsg);
    EXPECT_EQ(errMsg, "");

    std::vector<tagKeyValues> list = tagKeyValues::fromJson(arr);

    std::map<std::string, std::string> data{
            {"hello", "world"}
    };
    EXPECT_FALSE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
    EXPECT_TRUE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
}

TEST(Core_TaskManagerTest, tagKeyValuesIsDataValid02) {
    const char *szJson = R"([{
"label_name": "hello"
}])";
    std::string errMsg;
    json::Array arr = json::parseArray(szJson, errMsg);
    EXPECT_EQ(errMsg, "");

    std::vector<tagKeyValues> list = tagKeyValues::fromJson(arr);

    std::map<std::string, std::string> data{
            {"hello", "world"}
    };
    EXPECT_FALSE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
    EXPECT_TRUE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
}

TEST(Core_TaskManagerTest, tagKeyValuesIsDataValid03) {
    const char *szJson = R"([{
"label_name": "hello",
"label_values": ["* world"]
}])";
    std::string errMsg;
    json::Array arr = json::parseArray(szJson, errMsg);
    EXPECT_EQ(errMsg, "");

    std::vector<tagKeyValues> list = tagKeyValues::fromJson(arr);

    {
        std::map<std::string, std::string> data{
                {"hello", "world"}
        };
        EXPECT_TRUE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
        EXPECT_FALSE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
    }
    {
        std::map<std::string, std::string> data{
                {"hello", "hello world"}
        };
        EXPECT_FALSE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
        EXPECT_TRUE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
    }
}


TEST(Core_TaskManagerTest, tagKeyValuesIsDataValid04) {
    const char *szJson = R"([{
"label_name": "hello",
"label_values": ["!*"]
}])";
    std::string errMsg;
    json::Array arr = json::parseArray(szJson, errMsg);
    EXPECT_EQ(errMsg, "");

    std::vector<tagKeyValues> list = tagKeyValues::fromJson(arr);

    {
        std::map<std::string, std::string> data{
                {"hello", "world"}
        };
        EXPECT_TRUE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
        EXPECT_FALSE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
    }
}

TEST(Core_TaskManagerTest, tagKeyValuesIsDataValid05) {
    const char *szJson = R"([{
"label_name": "say",
"label_values": ["hello*"]
}])";
    std::string errMsg;
    json::Array arr = json::parseArray(szJson, errMsg);
    EXPECT_EQ(errMsg, "");

    std::vector<tagKeyValues> list = tagKeyValues::fromJson(arr);

    {
        std::map<std::string, std::string> data{
                {"say", "hello world"}
        };
        EXPECT_FALSE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
        EXPECT_TRUE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
    }
    {
        std::map<std::string, std::string> data{
                {"say", "1) hello"}
        };
        EXPECT_TRUE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
        EXPECT_FALSE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
    }
}


TEST(Core_TaskManagerTest, tagKeyValuesIsDataValid06) {
    const char *szJson = R"([{
"label_name": "hello",
"label_values": ["!"]
}])";
    std::string errMsg;
    json::Array arr = json::parseArray(szJson, errMsg);
    EXPECT_EQ(errMsg, "");

    std::vector<tagKeyValues> list = tagKeyValues::fromJson(arr);

    {
        std::map<std::string, std::string> data{
                {"hello", "world"}
        };
        EXPECT_FALSE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
        EXPECT_TRUE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
    }
    {
        std::map<std::string, std::string> data{
                {"hello", ""}
        };
        EXPECT_TRUE(tagKeyValues::IsDataValid(list, std::vector<tagKeyValues>(), data));
        EXPECT_FALSE(tagKeyValues::IsDataValid(std::vector<tagKeyValues>(), list, data));
    }
}

TEST(Core_TaskManagerTest, tagKeyValuesIsDataValid07) {
    struct {
        bool expect;
        const char *value;
        const char *patterns;
    } testCases[] = {
            {false, "/docker/hello", "!/docker/*\0",},
            {true,  "/docker",       "*\0",},
            {true,  "/docker",       "!\0",},
            {false, "",              "!\0",},
            {true,  "/docker/hello", "/docker/*\0",},
            {true,  "/docker/hello", "/docker/hello\0",},
            {true,  "test",          "!/docker/hello\0",},
            {false, "test",          "!/docker/hello\0/test/hello\0",},
            {true,  "/test/hello",   "!/docker/hello\0/test*\0",},
            {true,  "/test/hello",   "!/docker/hello\0/test1*\0/test/hello\0",},
            {false, "/test/hello",   "!/docker/hello\0/test1*\0!/test/*\0",},
    };
    int index = 0;
    for (const auto &tc: testCases) {
        index++;

        std::vector<std::string> values;
        for (const char *it = tc.patterns; *it; it += (strlen(it) + 1)) {
            values.emplace_back(it);
        }

        tagKeyValues kv("hello", values);
        LogDebug("[{:2}] pattern: <{}>, value: {}", index, StringUtils::join(kv.getValues(), "> | <"), tc.value);
        std::map<std::string, std::string> data{{"hello", tc.value}};
        EXPECT_EQ(tc.expect, kv.IsMatch(data));
    }
}
