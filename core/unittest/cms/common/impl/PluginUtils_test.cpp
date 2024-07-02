//
// Created by 韩呈杰 on 2023/7/2.
//
#include <gtest/gtest.h>
#include "common/PluginUtils.h"
#include "common/Logger.h"
#include "common/Arithmetic.h"
#include "common/UnitTestEnv.h"
#include "common/FileUtils.h"
#include "common/FilePathUtils.h"

using namespace std;
using namespace plugin;

TEST(PluginUtilsTest, DELTA) {
	int a = 10;
	int b = 3;
	int c = 11;
	EXPECT_EQ(Delta(a, b), 7);
	EXPECT_EQ(Delta(a, c), 0);
	EXPECT_EQ(RATIO(a + 3, 100 + c), 1.0 * (a + 3) / (100 + c));
	EXPECT_EQ(RATIO(a, 0), 0.0);
	EXPECT_EQ(RATIO(-2, 10), 0.0);
	EXPECT_EQ(PERCENT(a + 3, 100 + c), 100.0 * (a + 3) / (100 + c));
	EXPECT_EQ(PERCENT(a, 0), 0.0);
	EXPECT_EQ(PERCENT(-2, 10), 0.0);
}

TEST(PluginUtilsTest, SubDirs) {
	auto targetPath = (TEST_CONF_PATH / "conf").string();
	std::vector<std::string> subDirs;
	EXPECT_GT(PluginUtils::GetSubDirs(targetPath, subDirs), 0);

	LogInfo("ls {}", targetPath);
	int count = 0;
	for (const auto &dir : subDirs) {
		LogInfo("[{:2d}] {}", ++count, dir);
	}
}

TEST(PluginUtilsTest, getKeys) {
	std::vector<std::string> keys;
	EXPECT_EQ(-1, PluginUtils::getKeys("", keys));
	EXPECT_EQ(-1, PluginUtils::getKeys("\t ", keys));

	EXPECT_EQ(2, PluginUtils::getKeys("123 abc", keys));
	std::vector<std::string> expect{ "123", "abc" };
	EXPECT_EQ(expect, keys);
}

TEST(PluginUtilsTest, getValues) {
	std::vector<long> keys;
	EXPECT_EQ(-1, PluginUtils::getValues("", keys));
	EXPECT_TRUE(keys.empty());
	keys.clear();

	EXPECT_EQ(-1, PluginUtils::getValues("\t ", keys));
	EXPECT_TRUE(keys.empty());
	keys.clear();

	EXPECT_EQ(-1, PluginUtils::getValues("123 abc", keys));
	EXPECT_EQ(size_t(1), keys.size());
	EXPECT_EQ(long(123), keys[0]);
}

TEST(PluginUtilsTest, GetNumberFromFile) {
	fs::path dir = TEST_CONF_PATH / "conf" / "tmp";
	if (!fs::exists(dir)) {
		fs::create_directories(dir);
	}
	ASSERT_TRUE(fs::exists(dir));
	fs::path file = dir / fmt::format("tmp-number-{}.txt", __LINE__);
	if (fs::exists(file)) {
		fs::remove(file);
	}
	ASSERT_FALSE(fs::exists(file));
	LogInfo("FILE: {}", file.string());
	auto n = PluginUtils::GetNumberFromFile(file.string());
	EXPECT_EQ(n, decltype(n)(0));

	ASSERT_TRUE(WriteFileContent(file, fmt::format("{}", 123)));
	EXPECT_EQ(uint64_t(123), PluginUtils::GetNumberFromFile(file.string()));
	EXPECT_EQ(int64_t(123), PluginUtils::GetSignedNumberFromFile(file.string()));
}
