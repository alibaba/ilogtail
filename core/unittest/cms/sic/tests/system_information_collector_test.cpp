//
// Created by 韩呈杰 on 2023/2/4.
//
#include <gtest/gtest.h>
#include "sic/system_information_collector.h"

#include "common/StringUtils.h"   // sout
#include "common/FilePathUtils.h" // fs::exists

using namespace std::chrono;

TEST(SicSystemInformationCollectorTest, SicCalculateCpuPerc) {
    SicCpuInformation prev;
    prev.idle = milliseconds{90};
    prev.sys = milliseconds{1};

    SicCpuInformation curr;
    curr.idle = milliseconds{99};
    curr.sys = milliseconds{10};

    SicCpuPercent percent = prev / curr;
    EXPECT_EQ(0.5, percent.idle);
    std::cout << "idle: " << percent.idle << std::endl;

    curr.reset();
    EXPECT_EQ(0, curr.idle.count());
    EXPECT_EQ(0, curr.sys.count());
}

TEST(SicSystemInformationCollectorTest, SicGetFileSystemType) {
    {
        SicFileSystemType fsType{SIC_FILE_SYSTEM_TYPE_UNKNOWN};
        std::string name;
        EXPECT_TRUE(SystemInformationCollector::SicGetFileSystemType("fat32", fsType, name));
        EXPECT_EQ(SIC_FILE_SYSTEM_TYPE_LOCAL_DISK, fsType);
        EXPECT_EQ(name, "local");
    }
    {
        SicFileSystemType fsType{SIC_FILE_SYSTEM_TYPE_UNKNOWN};
        std::string name;
        EXPECT_TRUE(SystemInformationCollector::SicGetFileSystemType("ntfs", fsType, name));
        EXPECT_EQ(SIC_FILE_SYSTEM_TYPE_LOCAL_DISK, fsType);
        EXPECT_EQ(name, "local");
    }
    {
        SicFileSystemType fsType{SIC_FILE_SYSTEM_TYPE_UNKNOWN};
        std::string name;
        EXPECT_FALSE(SystemInformationCollector::SicGetFileSystemType("~~~~~~", fsType, name));
        EXPECT_EQ(SIC_FILE_SYSTEM_TYPE_NONE, fsType);
        EXPECT_EQ(name, "none");
    }
}

TEST(SicSout, Append) {
    std::string s = (sout{} << "hello").str();
    EXPECT_EQ(s, "hello");
}

TEST(SicSystemInformationCollectorTest, POD) {
#define SIZEOF(S) std::cout << "sizeof(" << #S << ") => " << sizeof(S) << std::endl;

    SIZEOF(SicNetConnectionInformation)
    SIZEOF(SicProcessCpuInformation)
    SIZEOF(SicProcessTime)
    SIZEOF(SicProcessCpuInformationCache)

    SicProcessTime obj;
    EXPECT_EQ(system_clock::time_point{}, obj.startTime);
}

TEST(SicOStringStream, FileSystemType_GetName) {
    for (int i = 0; i < SIC_FILE_SYSTEM_TYPE_MAX; i++) {
        EXPECT_NE(GetName(static_cast<SicFileSystemType>(i)), "");
    }
    EXPECT_EQ(GetName(static_cast<SicFileSystemType>(-1)), "");
    EXPECT_EQ(GetName(SIC_FILE_SYSTEM_TYPE_MAX), "");
}

TEST(SicCpuPercent, string) {
    SicCpuPercent cp;
    cp.sys = 10;
    std::string s = cp.string();
    EXPECT_NE(s.find("sys: 10"), std::string::npos);
    std::cout << s << std::endl;
}

TEST(SicDiskUsageTest, string) {
    SicDiskUsage disk;
    EXPECT_NE(disk.string(), "");
}

TEST(SicCpuInformation, Add) {
    SicCpuInformation l, r;
    l.idle = milliseconds{1};
    r.idle = milliseconds{2};

    l += r;
    EXPECT_EQ(milliseconds{3}, l.idle);
}

TEST(SicCpuBaseTest, zip) {
    CpuBase<double> cpu1;
    CpuBase<int> cpu2;

    std::set<std::string> nameSet;
    cpu1.zip(cpu2, [&](const char* name, double& a, const int& b) {
        nameSet.insert(name);
    });
    EXPECT_EQ(nameSet.size(), sizeof(cpu1) / sizeof(double));
}

TEST(SicCpuInformationTest, str) {
    SicCpuInformation cpu;
    {
        std::string expect = R"(SicCpuInformation{ user: 0µs, sys: 0µs, nice: 0µs, idle: 0µs, wait: 0µs, irq: 0µs, softIrq: 0µs, stolen: 0µs, total: 0µs, })";
#ifdef WIN32
        expect = "struct " + expect;
#endif
        // std::cout << cpu.str(true, " ") << std::endl;
        EXPECT_EQ(expect, cpu.str(true, " "));
    }
    {
        const std::string expect = R"({
   user: 0µs,
    sys: 0µs,
   nice: 0µs,
   idle: 0µs,
   wait: 0µs,
    irq: 0µs,
softIrq: 0µs,
 stolen: 0µs,
  total: 0µs,
})";
        // std::cout << cpu.str() << std::endl;
        EXPECT_EQ(expect, cpu.str());
    }
}

#if defined(WIN32) || defined(__linux__) || defined(__unix__)
TEST(SicSystemInformationCollectorTest, DevSerialId) {
#ifdef WIN32
#   define device "C:"
#else
#   define device "/dev/vda"
    if (!fs::exists(device)) {
        return; // 设备不存在，则跳过
    }
#endif
    auto collector = SystemInformationCollector::New();
    std::string sn;
    EXPECT_EQ(collector->SicGetDiskSerialId(device, sn), SIC_EXECUTABLE_SUCCESS);
    EXPECT_FALSE(sn.empty());
    std::cout << "device: " << device << ", serialId: " << sn << std::endl;
}
#endif // defined(WIN32) || defined(__linux__) || defined(__unix__)
