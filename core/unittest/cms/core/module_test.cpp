#include <gtest/gtest.h>
#include "core/module.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "common/ModuleData.h"
#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "sic/system_information_collector_util.h"

#if !defined(ENABLE_CLOUD_MONITOR)
#   include "common/ScopeGuard.h"
#endif

using namespace std;
using namespace common;
using namespace argus;

class CoreModuleTest : public testing::Test {
};


TEST_F(CoreModuleTest, Module) {
#ifdef ENABLE_CLOUD_MONITOR
#   define MODULE_TYPE MODULE_COLLECT_TYPE
#   define ARGS "1"
#   define MIN_LEN 0
#   define EXPECT_MODULE_NAME "cpu"
#else
#   define MODULE_TYPE MODULE_NOT_SO_TYPE
#   define ARGS ""
#   define MIN_LEN 10
#   define EXPECT_MODULE_NAME "cpu_second"
#endif

    Module pModule("cpu", "cpu_second", ARGS, MODULE_TYPE);
    EXPECT_TRUE(pModule.Init());
    char *buf = nullptr;
    EXPECT_EQ(0, pModule.Collect(buf));

    std::this_thread::sleep_for(std::chrono::seconds{1});

    int len = pModule.Collect(buf);
    ASSERT_GT(len, MIN_LEN);
    ASSERT_EQ(len, int(strlen(buf)));
    CollectData collectData;
    ModuleData::convertStringToCollectData(buf, collectData);

    EXPECT_EQ(collectData.moduleName, EXPECT_MODULE_NAME);
    cout << buf << endl;

    pModule.FreeCollectBuf(buf);

#undef EXPECT_MODULE_NAME
#undef MIN_LEN
#undef ARGS
#undef EXPECT_TYPE
}

#if !defined(ENABLE_CLOUD_MONITOR)
TEST_F(CoreModuleTest, Init001) {
    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    const char *szModuleName = "!@#$%^&*";
    Module module(szModuleName, ")(*&^%", nullptr, MODULE_NOT_SO_TYPE);
    EXPECT_FALSE(module.Init());

    auto cache = logger->getLogCache(true);
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find(fmt::format("static non-cms module ({}): can't find", szModuleName)));
}

TEST_F(CoreModuleTest, Init002) {
    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    const char *szModuleName = "!@#$%^&*";
    Module module(szModuleName, ")(*&^%", nullptr, MODULE_SHENNONG_TYPE);
    EXPECT_FALSE(module.Init());

    auto cache = logger->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("load so module"));
    EXPECT_NE(std::string::npos, cache.find("error"));
}

TEST_F(CoreModuleTest, Init003) {
    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    const char *szModuleName = "!@#$%^&*";
    Module module(szModuleName, ")(*&^%", nullptr, MODULE_COLLECT_TYPE);
    EXPECT_FALSE(module.Init());

    auto cache = logger->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("load so module"));
    EXPECT_NE(std::string::npos, cache.find("error"));
}

TEST_F(CoreModuleTest, Send) {
    Module module("unknown", "unknown", nullptr, MODULE_COLLECT_TYPE);
    EXPECT_EQ(nullptr, module.module_send);
    EXPECT_EQ(-1, module.Send(nullptr, 0, nullptr, 0, nullptr, nullptr, std::chrono::system_clock::time_point{}));

    struct Stub {
        static int Send(const char *,double ,const char *,int ,const char *,const char *, int) {
            return 1;
        }
    };
    module.module_send = &Stub::Send;
    EXPECT_EQ(1, module.Send(nullptr, 0, nullptr, 0, nullptr, nullptr, std::chrono::system_clock::time_point{}));
}

TEST_F(CoreModuleTest, SendData) {
    Module module("unknown", "unknown", nullptr, MODULE_COLLECT_TYPE);
    EXPECT_EQ(nullptr, module.module_send_data);
    EXPECT_EQ(-1, module.SendData(nullptr, 0));

    struct Stub {
        static int SendData(const char *, int) {
            return 1;
        }
    };
    module.module_send_data = &Stub::SendData;
    EXPECT_EQ(1, module.SendData(nullptr, 0));
}
#endif  // !ENABLE_CLOUD_MONITOR

TEST_F(CoreModuleTest, SendMetric) {
    Module module("unknown", "unknown", nullptr, MODULE_COLLECT_TYPE);
    EXPECT_EQ(nullptr, module.module_send_metric);
    const char *nil = "nullptr";
    EXPECT_EQ(-1, module.SendMetric(nil, std::chrono::system_clock::time_point{}, nil, nil, nil));

    struct Stub {
        static int SendMetric(const char *, int, const char *, const char *, const char *) {
            return 1;
        }
    };
    module.module_send_metric = &Stub::SendMetric;
    EXPECT_EQ(1, module.SendMetric(nullptr, std::chrono::system_clock::time_point{}, nullptr, nullptr, nullptr));
}

TEST_F(CoreModuleTest, Collect001) {
    Module module("unknown", "unknown", nullptr, MODULE_COLLECT_TYPE);
    EXPECT_EQ(nullptr, module.module_collect);
    char *buf = nullptr;
    EXPECT_EQ(-1, module.Collect(buf));
}

#ifndef ENABLE_CLOUD_MONITOR
TEST_F(CoreModuleTest, LoadSo) {
    fs::path soCpu = TEST_CONF_PATH.parent_path().parent_path() / "package" / "lib" / "libcpu.so";
    LogInfo("{}", soCpu.string());
    if (fs::exists(soCpu)) {
        Module module("cpu", "cpu", nullptr, MODULE_COLLECT_TYPE);
        module.mModulePath = soCpu.string();
        module.mModuleType = MODULE_COLLECT_TYPE;
        EXPECT_TRUE(module.Init());
    }
}
#endif
