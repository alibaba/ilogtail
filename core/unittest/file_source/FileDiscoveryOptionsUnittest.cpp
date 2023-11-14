// Copyright 2023 iLogtail Authors
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

#include <filesystem>
#include <memory>
#include <string>

#include "json/json.h"

#include "file_server/FileDiscoveryOptions.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class FileDiscoveryOptionsUnittest : public testing::Test {
public:
    void OnSuccessfulInit() const;
    void OnFailedInit() const;

private:
    bool ParseConfig(const string& config, Json::Value& res) const;

    const string pluginName = "test";
};

void FileDiscoveryOptionsUnittest::OnSuccessfulInit() const {
    unique_ptr<FileDiscoveryOptions> config;
    Json::Value configJson;
    string configStr;
    PipelineContext ctx;
    filesystem::path filePaths = filesystem::absolute("*.log");

    // only mandatory param
    configStr = R"(
        {
            "FilePath": []
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson));
    configJson["FilePath"].append(Json::Value(filePaths.string()));
    config.reset(new FileDiscoveryOptions());
    APSARA_TEST_TRUE(config->Init(configJson, ctx, pluginName));
    APSARA_TEST_EQUAL(1, config->mFilePaths.size());
    APSARA_TEST_EQUAL(0, config->mMaxDirSearchDepth);
    APSARA_TEST_EQUAL(-1, config->mPreservedDirDepth);
    APSARA_TEST_EQUAL(0, config->mExcludeFilePaths.size());
    APSARA_TEST_EQUAL(0, config->mExcludeFiles.size());
    APSARA_TEST_EQUAL(0, config->mExcludeDirs.size());
    APSARA_TEST_FALSE(config->mAllowingCollectingFilesInRootDir);
    APSARA_TEST_FALSE(config->mAllowingIncludedByMultiConfigs);

    // valid optional param
    configStr = R"(
        {
            "FilePath": [],
            "MaxDirSearchDepth": 1,
            "PreservedDirDepth": 0,
            "ExcludeFilePaths": ["/home/b.log"],
            "ExcludeFiles": ["a.log"],
            "ExcludeDirs": ["/home/test"],
            "AllowingCollectingFilesInRootDir": true,
            "AllowingIncludedByMultiConfigs": true
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson));
    configJson["FilePath"].append(Json::Value(filePaths.string()));
    config.reset(new FileDiscoveryOptions());
    APSARA_TEST_TRUE(config->Init(configJson, ctx, pluginName));
    APSARA_TEST_EQUAL(1, config->mFilePaths.size());
    APSARA_TEST_EQUAL(1, config->mMaxDirSearchDepth);
    APSARA_TEST_EQUAL(0, config->mPreservedDirDepth);
    APSARA_TEST_EQUAL(1, config->mExcludeFilePaths.size());
    APSARA_TEST_EQUAL(1, config->mExcludeFiles.size());
    APSARA_TEST_EQUAL(1, config->mExcludeDirs.size());
    APSARA_TEST_TRUE(config->mAllowingCollectingFilesInRootDir);
    APSARA_TEST_TRUE(config->mAllowingIncludedByMultiConfigs);

    // invalid optional param
    configStr = R"(
        {
            "FilePath": [],
            "MaxDirSearchDepth": true,
            "PreservedDirDepth": true,
            "ExcludeFilePaths": true,
            "ExcludeFiles": true,
            "ExcludeDirs": true,
            "AllowingCollectingFilesInRootDir": "true",
            "AllowingIncludedByMultiConfigs": "true"
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson));
    configJson["FilePath"].append(Json::Value(filePaths.string()));
    config.reset(new FileDiscoveryOptions());
    APSARA_TEST_TRUE(config->Init(configJson, ctx, pluginName));
    APSARA_TEST_EQUAL(1, config->mFilePaths.size());
    APSARA_TEST_EQUAL(0, config->mMaxDirSearchDepth);
    APSARA_TEST_EQUAL(-1, config->mPreservedDirDepth);
    APSARA_TEST_EQUAL(0, config->mExcludeFilePaths.size());
    APSARA_TEST_EQUAL(0, config->mExcludeFiles.size());
    APSARA_TEST_EQUAL(0, config->mExcludeDirs.size());
    APSARA_TEST_FALSE(config->mAllowingCollectingFilesInRootDir);
    APSARA_TEST_FALSE(config->mAllowingIncludedByMultiConfigs);

    // ExcludeFilePaths
    filesystem::path ex1 = filesystem::path(".") / "test" / "a.log";  // not absolute
    filesystem::path ex2 = filesystem::current_path() / "**" / "b.log"; // ML
    filesystem::path ex3 = filesystem::absolute(ex1);
    configStr = R"(
        {
            "FilePath": [],
            "ExcludeFilePaths": [],
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson));
    configJson["FilePath"].append(Json::Value(filePaths.string()));
    configJson["ExcludeFilePaths"].append(Json::Value(ex1.string()));
    configJson["ExcludeFilePaths"].append(Json::Value(ex2.string()));
    configJson["ExcludeFilePaths"].append(Json::Value(ex3.string()));
    config.reset(new FileDiscoveryOptions());
    APSARA_TEST_TRUE(config->Init(configJson, ctx, pluginName));
    APSARA_TEST_EQUAL(3, config->mExcludeFilePaths.size());
    APSARA_TEST_EQUAL(1, config->mFilePathBlacklist);
    APSARA_TEST_EQUAL(1, config->mMLFilePathBlacklist);
    APSARA_TEST_TRUE(config->mHasBlacklist);

    // ExcludeFiles
    filesystem::path ex1 = "a.log";
    filesystem::path ex2 = filesystem::current_path() / "b.log"; // has path separator
    configStr = R"(
        {
            "FilePath": [],
            "ExcludeFiles": [],
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson));
    configJson["FilePath"].append(Json::Value(filePaths.string()));
    configJson["ExcludeFiles"].append(Json::Value(ex1.string()));
    configJson["ExcludeFiles"].append(Json::Value(ex2.string()));
    config.reset(new FileDiscoveryOptions());
    APSARA_TEST_TRUE(config->Init(configJson, ctx, pluginName));
    APSARA_TEST_EQUAL(2, config->mExcludeFiles.size());
    APSARA_TEST_EQUAL(1, config->mFileNameBlacklist);
    APSARA_TEST_TRUE(config->mHasBlacklist);

    // ExcludeDirs
    filesystem::path ex1 = filesystem::path(".") / "test"; // not absolute
    filesystem::path ex2 = filesystem::current_path() / "**" / "test"; // ML
    filesystem::path ex3 = filesystem::current_path() / "a*"; // *
    filesystem::path ex4 = filesystem::current_path() / "a?"; // ?
    filesystem::path ex5 = filesystem::absolute(ex1);
    configStr = R"(
        {
            "FilePath": [],
            "ExcludeDirs": [],
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson));
    configJson["FilePath"].append(Json::Value(filePaths.string()));
    configJson["ExcludeDirs"].append(Json::Value(ex1.string()));
    configJson["ExcludeDirs"].append(Json::Value(ex2.string()));
    configJson["ExcludeDirs"].append(Json::Value(ex3.string()));
    configJson["ExcludeDirs"].append(Json::Value(ex4.string()));
    configJson["ExcludeDirs"].append(Json::Value(ex5.string()));
    config.reset(new FileDiscoveryOptions());
    APSARA_TEST_TRUE(config->Init(configJson, ctx, pluginName));
    APSARA_TEST_EQUAL(4, config->mExcludeDirs.size());
    APSARA_TEST_EQUAL(1, config->mMLWildcardDirPathBlacklist);
    APSARA_TEST_EQUAL(1, config->mWildcardDirPathBlacklist);
    APSARA_TEST_EQUAL(1, config->mDirPathBlacklist);
    APSARA_TEST_TRUE(config->mHasBlacklist);

    // AllowingCollectingFilesInRootDir
    configStr = R"(
        {
            "FilePath": [],
            "AllowingCollectingFilesInRootDir": true,
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson));
    configJson["FilePath"].append(Json::Value(filePaths.string()));
    config.reset(new FileDiscoveryOptions());
    APSARA_TEST_TRUE(config->Init(configJson, ctx, pluginName));
    APSARA_TEST_FALSE(config->mAllowingIncludedByMultiConfigs);
    APSARA_TEST_FALSE(BOOL_FLAG(enable_root_path_collection));
}

bool FileDiscoveryOptionsUnittest::ParseConfig(const string& config, Json::Value& res) const {
    Json::CharReaderBuilder builder;
    const unique_ptr<Json::CharReader> reader(builder.newCharReader());
    string errorMsg;
    return reader->parse(config.c_str(), config.c_str() + config.size(), &res, &errorMsg);
}

UNIT_TEST_CASE(FileDiscoveryOptionsUnittest, OnSuccessfulInit)

} // namespace logtail

UNIT_TEST_MAIN
