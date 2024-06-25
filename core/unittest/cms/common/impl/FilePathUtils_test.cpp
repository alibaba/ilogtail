//
// Created by 韩呈杰 on 2023/5/19.
//
#include <gtest/gtest.h>
#include "common/FilePathUtils.h"
#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "common/TimeFormat.h"
#include "common/ChronoLiteral.h"
#include "common/Uuid.h"

using namespace common;

TEST(CommonFilePathUtilsTest, GetFileMTime) {
    auto file = TEST_CONF_PATH / "conf" / "conf.properties";
    auto mtime = FilePathUtils::GetFileMTime(file.string());
    LogInfo("<{}> mtime: {}", file.string(), date::format<3>(mtime));
    EXPECT_NE(0, mtime.time_since_epoch().count());
}

TEST(CommonFilePathUtilsTest, GetLastFile) {
    using namespace std::chrono;
    auto now = system_clock::now() - seconds{1000};
	fs::path root = TEST_CONF_PATH / "conf" / "logCollect" / "logs";

    EXPECT_TRUE(FilePathUtils::SetFileMTime(root / "test.log", now));
    EXPECT_TRUE(FilePathUtils::SetFileMTime(root / "test.log.1", now - 100_s));
    EXPECT_TRUE(FilePathUtils::SetFileMTime(root / "test.log.2", now - 300_s));
    EXPECT_TRUE(FilePathUtils::SetFileMTime(root / "twoLine.log", now)); // 不符合通配

    std::string targetFile = FilePathUtils::GetLastFile(root.string(), "test.log", now - 1500_s);
    EXPECT_EQ(targetFile, (root / "test.log.1").string());
}

TEST(CommonFilePathUtilsTest, GetExecPath) {
    fs::path execPath{GetExecPath()};
    EXPECT_FALSE(execPath.empty());
    std::cout << execPath << std::endl;
    std::cout << execPath.parent_path() << std::endl;  // 裁掉了文件名
    std::cout << execPath.parent_path().parent_path() << std::endl; // 进入上一级目录
}

TEST(CommonFilePathUtilsTest, GetBaseDir) {
    std::cout << "--- Base Directory: " << GetBaseDir() << std::endl;
    std::string baseDir = GetBaseDir().string();
    if (!baseDir.find("unit_test")) {
        EXPECT_NE(baseDir.find("bin"), std::string::npos);
    }
}

TEST(CommonFilePathUtilsTest, Join) {
    fs::path path("/usr");
    path = path / 1;
#ifdef WIN32
    EXPECT_EQ("/usr\\1", path.string());
#else
    EXPECT_EQ("/usr/1", path.string());
#endif
}
TEST(CommonFilePathUtilsTest, Join1) {
    fs::path path("/usr");
    auto dest = path / "include";
#ifdef WIN32
    EXPECT_EQ(dest.string(), "/usr\\include");
#else
    EXPECT_EQ(dest.string(), "/usr/include");
#endif
    auto dest2 = path / "/include";
    EXPECT_EQ(dest2.string(), "/usr/include");
    EXPECT_EQ(dest2.parent_path().string(), "/usr");

#ifdef WIN32
    auto dest3 = path / "\\include";
    EXPECT_EQ(dest3.string(), "/usr\\include");
#endif
}

TEST(CommonFilePathUtilsTest, Join2) {
    fs::path path("/usr/");
    auto dest = path / "include";
    EXPECT_EQ(dest.string(), "/usr/include");
    EXPECT_FALSE(dest.empty());

    auto dest2 = path / "/include";
    EXPECT_EQ(dest2.string(), "/usr//include");
}

TEST(CommonFilePathUtilsTest, Join3) {
    fs::path path("");
    auto dest = path / "include";
    EXPECT_EQ(dest.string(), "include");

    auto dest2 = fs::path("/usr") / "";
    EXPECT_EQ(dest2.string(), "/usr");
}

TEST(CommonFilePathUtilsTest, Path_IsAbsolute) {
#ifdef WIN32
    {
        fs::path p(R"(C:\)");
        EXPECT_TRUE(p.is_absolute());
        EXPECT_FALSE(p.is_relative());
    }
    {
        fs::path p("x:/");
        EXPECT_TRUE(p.is_absolute());
        EXPECT_FALSE(p.is_relative());
    }
    {
        fs::path p(R"(\\User)");
        EXPECT_FALSE(p.is_absolute());
        EXPECT_TRUE(p.is_relative());
    }
    {
        fs::path p(R"(\\)");
        EXPECT_FALSE(p.is_absolute());
        EXPECT_TRUE(p.is_relative());
    }
    {
        fs::path p(R"(\Users)");
        EXPECT_FALSE(p.is_absolute());
        EXPECT_TRUE(p.is_relative());
    }
#else
    {
        fs::path p("/");
        EXPECT_TRUE(p.is_absolute());
        EXPECT_FALSE(p.is_relative());
    }
    {
        fs::path p("./");
        EXPECT_FALSE(p.is_absolute());
        EXPECT_TRUE(p.is_relative());
    }
#endif
}

TEST(CommonFilePathUtilsTest, has_root_name) {
    EXPECT_FALSE(fs::path{"/usr"}.has_root_name());
    EXPECT_TRUE(fs::path{"//net/"}.has_root_name());
}

TEST(CommonFilePathUtilsTest, RemoveFile) {
    // 删除不存在的文件，返回true
    EXPECT_TRUE(RemoveFile("/tmp/" + NewUuid() + ".txt"));
    EXPECT_TRUE(RemoveFile("smb://127.0.0.1/tmp/" + NewUuid() + ".txt"));
}