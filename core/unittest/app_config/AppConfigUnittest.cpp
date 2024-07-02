// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "app_config/AppConfig.h"
#include "common/FileSystemUtil.h"
#include "common/JsonUtil.h"
#include "unittest/Unittest.h"


DECLARE_FLAG_STRING(ilogtail_config);


namespace logtail {

class AppConfigUnittest : public ::testing::Test {
public:
    void TestLoadEbpfParameters();

    void TestDefaultAndLoadEbpfParameters();

    void TestDefaultEbpfParameters();

protected:
    void TearDown() override {
        AppConfig* appConfig = AppConfig::GetInstance();
        appConfig->mReceiveEventChanCap = 4096;
        appConfig->mAdminConfig.mDebugMode = false;
        appConfig->mAdminConfig.mLogLevel = "warn";
        appConfig->mAdminConfig.mPushAllSpan = false;
        appConfig->mAggregationConfig.mAggWindowSecond = 15;
        appConfig->mConverageConfig.mStrategy = "combine";
        appConfig->mSampleConfig.mStrategy = "fixedRate";
        appConfig->mSampleConfig.mConfig.mRate = 0.01;
        appConfig->mSocketProbeConfig.mSlowRequestThresholdMs = 500;
        appConfig->mSocketProbeConfig.mMaxConnTrackers = 10000;
        appConfig->mSocketProbeConfig.mMaxBandWidthMbPerSec = 30;
        appConfig->mSocketProbeConfig.mMaxRawRecordPerSec = 100000;
        appConfig->mProfileProbeConfig.mProfileSampleRate = 10;
        appConfig->mProfileProbeConfig.mProfileUploadDuration = 10;
        appConfig->mProcessProbeConfig.mEnableOOMDetect = false;
    }

private:
    void writeLogtailConfigJSON(const Json::Value& v) {
        LOG_INFO(sLogger, ("writeLogtailConfigJSON", v.toStyledString()));
        OverwriteFile(STRING_FLAG(ilogtail_config), v.toStyledString());
    }
};

void AppConfigUnittest::TestLoadEbpfParameters() {
    Json::Value value;
    std::string configStr, errorMsg;
    // valid optional param
    configStr = R"(
        {
            "ReceiveEventChanCap": 1024,
            "AdminConfig": {
                "DebugMode": true,
                "LogLevel": "error",
                "PushAllSpan": true
            },
            "AggregationConfig": {
                "AggWindowSecond": 8
            },
            "ConverageConfig": {
                "Strategy": "combine1"
            },
            "SampleConfig": {
                "Strategy": "fixedRate1",
                "Config": {
                    "Rate": 0.001
                }
            },
            "SocketProbeConfig": {
                "SlowRequestThresholdMs": 5000,
                "MaxConnTrackers": 100000,
                "MaxBandWidthMbPerSec": 300,
                "MaxRawRecordPerSec": 1000000
            },
            "ProfileProbeConfig": {
                "ProfileSampleRate": 100,
                "ProfileUploadDuration": 100
            },
            "ProcessProbeConfig": {
                "EnableOOMDetect": true
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, value, errorMsg));
    value["logtail_sys_conf_dir"] = GetProcessExecutionDir();
    writeLogtailConfigJSON(value);

    AppConfig* appConfig = AppConfig::GetInstance();
    appConfig->LoadAppConfig(STRING_FLAG(ilogtail_config));
    APSARA_TEST_EQUAL(appConfig->GetReceiveEventChanCap(), 1024);
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mDebugMode, true);
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mLogLevel, "error");
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mPushAllSpan, true);
    APSARA_TEST_EQUAL(appConfig->GetAggregationConfig().mAggWindowSecond, 8);
    APSARA_TEST_EQUAL(appConfig->GetConverageConfig().mStrategy, "combine1");
    APSARA_TEST_EQUAL(appConfig->GetSampleConfig().mStrategy, "fixedRate1");
    APSARA_TEST_EQUAL(appConfig->GetSampleConfig().mConfig.mRate, 0.001);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mSlowRequestThresholdMs, 5000);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxConnTrackers, 100000);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxBandWidthMbPerSec, 300);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxRawRecordPerSec, 1000000);
    APSARA_TEST_EQUAL(appConfig->GetProfileProbeConfig().mProfileSampleRate, 100);
    APSARA_TEST_EQUAL(appConfig->GetProfileProbeConfig().mProfileUploadDuration, 100);
    APSARA_TEST_EQUAL(appConfig->GetProcessProbeConfig().mEnableOOMDetect, true);
}

void AppConfigUnittest::TestDefaultAndLoadEbpfParameters() {
    Json::Value value;
    std::string configStr, errorMsg;
    // valid optional param
    configStr = R"(
        {
            "ReceiveEventChanCap": 1024,
            "SampleConfig": {
                "Strategy": "fixedRate1",
                "Config": {
                    "Rate": 0.001
                }
            },
            "SocketProbeConfig": {
                "SlowRequestThresholdMs": 5000,
                "MaxConnTrackers": 100000,
                "MaxBandWidthMbPerSec": 300,
                "MaxRawRecordPerSec": 1000000
            },
            "ProfileProbeConfig": {
                "ProfileSampleRate": 100,
                "ProfileUploadDuration": 100
            },
            "ProcessProbeConfig": {
                "EnableOOMDetect": true
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, value, errorMsg));
    value["logtail_sys_conf_dir"] = GetProcessExecutionDir();
    writeLogtailConfigJSON(value);

    AppConfig* appConfig = AppConfig::GetInstance();
    appConfig->LoadAppConfig(STRING_FLAG(ilogtail_config));
    APSARA_TEST_EQUAL(appConfig->GetReceiveEventChanCap(), 1024);
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mDebugMode, false);
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mLogLevel, "warn");
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mPushAllSpan, false);
    APSARA_TEST_EQUAL(appConfig->GetAggregationConfig().mAggWindowSecond, 15);
    APSARA_TEST_EQUAL(appConfig->GetConverageConfig().mStrategy, "combine");
    APSARA_TEST_EQUAL(appConfig->GetSampleConfig().mStrategy, "fixedRate1");
    APSARA_TEST_EQUAL(appConfig->GetSampleConfig().mConfig.mRate, 0.001);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mSlowRequestThresholdMs, 5000);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxConnTrackers, 100000);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxBandWidthMbPerSec, 300);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxRawRecordPerSec, 1000000);
    APSARA_TEST_EQUAL(appConfig->GetProfileProbeConfig().mProfileSampleRate, 100);
    APSARA_TEST_EQUAL(appConfig->GetProfileProbeConfig().mProfileUploadDuration, 100);
    APSARA_TEST_EQUAL(appConfig->GetProcessProbeConfig().mEnableOOMDetect, true);
}

void AppConfigUnittest::TestDefaultEbpfParameters() {
    Json::Value value;
    value["logtail_sys_conf_dir"] = GetProcessExecutionDir();
    writeLogtailConfigJSON(value);
    AppConfig* appConfig = AppConfig::GetInstance();
    // appConfig->LoadAppConfig(STRING_FLAG(ilogtail_config));

    APSARA_TEST_EQUAL(appConfig->GetReceiveEventChanCap(), 4096);
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mDebugMode, false);
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mLogLevel, "warn");
    APSARA_TEST_EQUAL(appConfig->GetAdminConfig().mPushAllSpan, false);
    APSARA_TEST_EQUAL(appConfig->GetAggregationConfig().mAggWindowSecond, 15);
    APSARA_TEST_EQUAL(appConfig->GetConverageConfig().mStrategy, "combine");
    APSARA_TEST_EQUAL(appConfig->GetSampleConfig().mStrategy, "fixedRate");
    APSARA_TEST_EQUAL(appConfig->GetSampleConfig().mConfig.mRate, 0.01);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mSlowRequestThresholdMs, 500);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxConnTrackers, 10000);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxBandWidthMbPerSec, 30);
    APSARA_TEST_EQUAL(appConfig->GetSocketProbeConfig().mMaxRawRecordPerSec, 100000);
    APSARA_TEST_EQUAL(appConfig->GetProfileProbeConfig().mProfileSampleRate, 10);
    APSARA_TEST_EQUAL(appConfig->GetProfileProbeConfig().mProfileUploadDuration, 10);
    APSARA_TEST_EQUAL(appConfig->GetProcessProbeConfig().mEnableOOMDetect, false);
}

UNIT_TEST_CASE(AppConfigUnittest, TestDefaultEbpfParameters);
UNIT_TEST_CASE(AppConfigUnittest, TestDefaultAndLoadEbpfParameters);
UNIT_TEST_CASE(AppConfigUnittest, TestLoadEbpfParameters);

} // namespace logtail

UNIT_TEST_MAIN
