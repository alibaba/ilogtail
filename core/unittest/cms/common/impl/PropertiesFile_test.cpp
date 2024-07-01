#include <gtest/gtest.h>
#include "common/PropertiesFile.h"

#include "common/UnitTestEnv.h"
#include "common/Uuid.h"
#include "common/FileUtils.h" // WriteFileContent

using namespace common;

TEST(CommonPropertiesFileTest, test) {
    PropertiesFile pPro((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitor.conf").string());
    EXPECT_EQ("20", pPro.GetValue("detect.ping.thread", "10"));
    EXPECT_EQ("10", pPro.GetValue("detect.ping.thread1", "10"));
}

TEST(CommonPropertiesFileTest, AdjustPropertiesByJsonFile01) {
    PropertiesFile pPro((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitor.conf").string());
    EXPECT_EQ(0, pPro.AdjustPropertiesByJsonFile(TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitor.json"));
    EXPECT_EQ("15", pPro.GetValue("detect.ping.thread", "10"));
}

TEST(CommonPropertiesFileTest, AdjustPropertiesByJsonFileNotExist) {
    PropertiesFile pPro((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitor.conf").string());
    EXPECT_EQ(-1, pPro.AdjustPropertiesByJsonFile(TEST_CONF_PATH / "conf/cloudMonitor" / (NewUuid() + ".json")));
}

TEST(CommonPropertiesFileTest, AdjustPropertiesByJsonFile03) {
    PropertiesFile pPro((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitor.conf").string());
    fs::path jsonFile = TEST_CONF_PATH / "conf" / "tmp" / "invalid_gent_config.json";
    EXPECT_TRUE(WriteFileContent(jsonFile, ""));
    EXPECT_EQ(-2, pPro.AdjustPropertiesByJsonFile(jsonFile.string()));

    EXPECT_TRUE(WriteFileContent(jsonFile, "[]"));
    EXPECT_EQ(-3, pPro.AdjustPropertiesByJsonFile(jsonFile.string()));
}
