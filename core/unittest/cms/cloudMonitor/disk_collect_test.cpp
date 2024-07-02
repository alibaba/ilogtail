#include <gtest/gtest.h>
#include "cloudMonitor/disk_collect.h"

#include <iostream>
#include <thread>

#include "common/Config.h"

#include "common/SystemUtil.h"
#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/Payload.h"
#include "common/ModuleTool.h"
#include "common/ConsumeCpu.h"
#include "common/UnitTestEnv.h"
#include "common/ExecCmd.h"
#include "common/Defer.h"
#include "common/ChronoLiteral.h"
#ifdef WIN32
#include "common/FileUtils.h"
#endif

#include "cloudMonitor/cloud_module_macros.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;
using namespace std::chrono;

DECLARE_CMS_EXTERN_MODULE(disk);

class Cms_DiskCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();

        p_shared = new DiskCollect();
        p_shared->Init(15, "");
    }

    void TearDown() override {
        Delete(p_shared);
    }

    DiskCollect *p_shared = nullptr;
};

TEST_F(Cms_DiskCollectTest, mountPath) {
#ifdef WIN32
#   define PATH_SEP "\\"
#else
#   define PATH_SEP "/"
#endif
    EXPECT_EQ(p_shared->FormatDir("/test/"), PATH_SEP "test" PATH_SEP);
    EXPECT_EQ(p_shared->FormatDir(""), "");
    map<string, bool> excludeMountPathMap;
    EXPECT_EQ(1, p_shared->GetExcludeMountPathMap("/test", excludeMountPathMap));
    EXPECT_EQ(excludeMountPathMap[PATH_SEP "test" PATH_SEP], true);
    excludeMountPathMap.clear();
    EXPECT_EQ(1, p_shared->GetExcludeMountPathMap("/test", excludeMountPathMap));
    EXPECT_EQ(p_shared->IsCollectMountPath(excludeMountPathMap, "/test/hello"), false);
    excludeMountPathMap.clear();
    string excludeDir = R"([{"dir":"/var/lib/container"},{"dir":"/docker","includeSubDir":true}])";
    EXPECT_EQ(2, p_shared->GetExcludeMountPathMap(excludeDir, excludeMountPathMap));
    EXPECT_EQ(p_shared->IsCollectMountPath(excludeMountPathMap, "/var/lib/container"), false);
    EXPECT_EQ(p_shared->IsCollectMountPath(excludeMountPathMap, "/var/lib/container/test"), true);
    EXPECT_EQ(p_shared->IsCollectMountPath(excludeMountPathMap, "/docker"), false);
    EXPECT_EQ(p_shared->IsCollectMountPath(excludeMountPathMap, "/docker/test"), false);
#undef PATH_SEP
}

TEST_F(Cms_DiskCollectTest, GetDiskCollectStatMap) {
    map<string, DiskCollectStat> diskCollectStatMap;
    int re;
    string result;
    re = p_shared->GetDiskCollectStatMap(diskCollectStatMap);
#ifndef WIN32
    SystemUtil::execCmd("df -h|awk '{print $1}' |grep -c /", result);
    EXPECT_EQ(convert<int>(result), re);
#endif
    auto collectTime1 = p_shared->mDeviceMountMapExpireTime; // mLastDeviceMountCollectTime;
    std::this_thread::sleep_for(1_s);
    re = p_shared->GetDiskCollectStatMap(diskCollectStatMap);
#ifndef WIN32
    SystemUtil::execCmd("df -h|awk '{print $1}' |grep -c /", result);
    EXPECT_EQ(convert<int>(result), re);
#endif
    EXPECT_EQ(collectTime1, p_shared->mDeviceMountMapExpireTime);
}

TEST_F(Cms_DiskCollectTest, Collect) {
    p_shared->mTotalCount = 2;

    bool collected = false;
    int index = 0;
    auto collect = [&]() {
        string collectStr;
        int collectLen = p_shared->Collect(collectStr);
        if (index < p_shared->mTotalCount) {
            EXPECT_EQ(collectLen, 0);
            EXPECT_EQ(p_shared->mCount, index);
        } else {
            collected = true;
            std::cout << "[COLLECT] " << collectStr << std::endl;
            EXPECT_GT(collectLen, 0);
            EXPECT_EQ(p_shared->mCount, 0);
            EXPECT_EQ(collectLen > 0, true);
            cout << "collectLen= " << collectLen << endl;
            //cout<<"collectResult= "<<collectStr<<endl;
            CollectData collectData;
            ModuleData::convertStringToCollectData(collectStr, collectData);
            EXPECT_EQ(collectData.moduleName, p_shared->mModuleName);
            collectData.print(std::cout);

            const char *keys[] = {"max_read_bytes", "max_write_bytes"};
            double rw_bytes = 0;
            for (auto const &it: collectData.dataVector) {
                if (it.metricName() == "system.io") {
                    for (const char *key: keys) {
                        auto targetValueIt = it.valueMap.find(key);
                        if (targetValueIt != it.valueMap.end()) {
                            rw_bytes += targetValueIt->second;
                        }
                    }
                }
            }
            EXPECT_GE(rw_bytes, 0);
        }
        index++;
    };

    const std::chrono::milliseconds millis{500};

    collect();
    std::this_thread::sleep_for(millis);
    collect();
    // read files
    [&]() {
#ifdef WIN32
        const uint64_t maxRead = 20 * 1024 * 1024;
        fs::path path{__FILE__};
        path = path.parent_path();
        uint64_t readSize = 0;
        defer(std::cout << "total read file size: " << (readSize >> 20) << " MB" << std::endl);
        while (readSize < maxRead) {
            for (const auto &it: fs::directory_iterator{path, fs::directory_options::skip_permission_denied}) {
                if (fs::is_regular_file(it.path())) {
                    readSize += ReadFileBinary(it.path()).size();
                    if (readSize >= maxRead) {
                        break;
                    }
                }
            }
        }
#else
        const char *pattern = "/usr/bin/dd if={} of=/dev/null bs=1M count=20";
        for (const auto &it: this->p_shared->mCurrentDiskCollectStatMap) {
            const std::string cmd = fmt::format(pattern, it.second.deviceMountInfo.devName);
            std::cout << cmd << std::endl;
            std::string out;
            int exitCode = ExecCmd(cmd, &out);
            std::cout << "Exit Code: " << exitCode << std::endl;
        }
#endif
    }();
    [&]() {
        fs::path tmpPath{TEST_CONF_PATH / "tmp.data.txt"};
        FILE *fp = fopen(tmpPath.string().c_str(), "wb");
        if (fp) {
            defer(fs::remove(tmpPath));
            defer(fclose(fp));

            char buf[1024];
            for (size_t i = 0; i < sizeof(buf); i++) {
                buf[i] = static_cast<char>(i);
            }
            fwrite(buf, 1, sizeof(buf), fp);
        }
    }();
    std::this_thread::sleep_for(millis);
    collect();

    EXPECT_TRUE(collected);
}

TEST_F(Cms_DiskCollectTest, GetDiskName) {
    EXPECT_EQ("sda", p_shared->GetDiskName("/dev/sda1"));
    EXPECT_EQ("sda", p_shared->GetDiskName("/dev/sda9"));
    EXPECT_EQ("sda", p_shared->GetDiskName("/dev/sda10"));
}

TEST_F(Cms_DiskCollectTest, disk) {
    EXPECT_EQ(-1, cloudMonitor_disk_collect(nullptr, nullptr));

    auto *handler = cloudMonitor_disk_init("10");
    defer(cloudMonitor_disk_exit(handler));

    EXPECT_NE(nullptr, handler);
    for (int i = 0; i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds{30});
        EXPECT_EQ(0, cloudMonitor_disk_collect(handler, nullptr));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{30});

    {
        char *data = nullptr;
        EXPECT_EQ(cloudMonitor_disk_collect(handler, &data) > 0, true);
        cout << "result: " << data << endl;
        EXPECT_TRUE(Contain(std::string(data), "metricName system.disk"));
        // OneAgent环境下没有id_serial
// #if defined(WIN32) || defined(__linux__)
//      EXPECT_NE(std::string(data).find("id_serial"), std::string::npos);
// #endif
    }
    
    for (int t = 1; t < 2; t++) {
        for (int i = 0; i < 9; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds{30});
            EXPECT_EQ(0, cloudMonitor_disk_collect(handler, nullptr));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{30});
        char *data = nullptr;
        EXPECT_GT(cloudMonitor_disk_collect(handler, &data), 0);
        cout << "result:" << data << endl;
    }
}

TEST_F(Cms_DiskCollectTest, join_n) {
    vector<string> strs;
    string str = "abcabc";
    strs.push_back(str);
    strs.push_back(str);
    strs.push_back(str);
    string result = join_n(strs, ",", 12);
    EXPECT_EQ(result, "abcabc");
    result = join_n(strs, ",", 13);
    EXPECT_EQ(result, "abcabc,abcabc");
    result = join_n(strs, ",", 15);
    EXPECT_EQ(result, "abcabc,abcabc");
    result = join_n(strs, ",", 2);
    EXPECT_EQ(result, "abcabc");  // 最少一条
    result = join_n(strs, ",", 25);
    EXPECT_EQ(result, "abcabc,abcabc,abcabc");
}

