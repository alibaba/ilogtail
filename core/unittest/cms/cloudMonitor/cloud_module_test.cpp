//
// Created by 韩呈杰 on 2023/2/5.
//
#include <gtest/gtest.h>
#include <string>
#include "cloudMonitor/cloud_module.h"
#include "common/ScopeGuard.h"

TEST(Cms_CloudModuleTest, normal) {
    argus::MODULE_INIT_FUNCTION fnInit = nullptr;
    argus::MODULE_COLLECT_FUNCTION fnCollect = nullptr;
    argus::MODULE_FREE_BUF_FUNCTION fnFreeBuf = nullptr;
    argus::MODULE_EXIT_FUNCTION fnExit = nullptr;

    EXPECT_TRUE(cloudMonitor::GetModule(fnInit, fnCollect, fnFreeBuf, fnExit, "cpu"));
    EXPECT_NE(nullptr, fnInit);
    EXPECT_NE(nullptr, fnCollect);
    EXPECT_NE(nullptr, fnFreeBuf);
    EXPECT_NE(nullptr, fnExit);
}

TEST(Cms_CloudModuleTest, invalidModule) {
    argus::MODULE_INIT_FUNCTION fnInit = nullptr;
    argus::MODULE_COLLECT_FUNCTION fnCollect = nullptr;
    argus::MODULE_FREE_BUF_FUNCTION fnFreeBuf = nullptr;
    argus::MODULE_EXIT_FUNCTION fnExit = nullptr;

    EXPECT_FALSE(cloudMonitor::GetModule(fnInit, fnCollect, fnFreeBuf, fnExit, "12345"));
    EXPECT_EQ(nullptr, fnInit);
    EXPECT_EQ(nullptr, fnCollect);
    EXPECT_EQ(nullptr, fnFreeBuf);
    EXPECT_EQ(nullptr, fnExit);
}

TEST(Cms_CloudModuleTest, Modules) {
    const char *modules[] = {
            "cpu", "memory", "detect", "disk", "gpu", "net", "netstat", "system", "process",
#if defined(__linux__) || defined(__unix__)
            "tcpExt", "tcp", "rdma",
#endif
    };
    for (const char *moduleName: modules) {
        argus::MODULE_INIT_FUNCTION fnInit = nullptr;
        argus::MODULE_COLLECT_FUNCTION fnCollect = nullptr;
        argus::MODULE_FREE_BUF_FUNCTION fnFreeBuf = nullptr;
        argus::MODULE_EXIT_FUNCTION fnExit = nullptr;

        std::cout << std::string(30, '-') << moduleName << std::string(90, '-') << std::endl;

        EXPECT_TRUE(cloudMonitor::GetModule(fnInit, fnCollect, fnFreeBuf, fnExit, moduleName));
        EXPECT_NE(nullptr, fnInit);
        EXPECT_NE(nullptr, fnCollect);
        EXPECT_NE(nullptr, fnFreeBuf);
        EXPECT_NE(nullptr, fnExit);

        IHandler *handler = fnInit(nullptr);
        EXPECT_NE(nullptr, handler);
        fnExit(handler);

        EXPECT_GT(cloudMonitor::GetActiveModuleCount(), 0);
    }
}
