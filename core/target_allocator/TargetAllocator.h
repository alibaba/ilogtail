/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <json/json.h>
#include <shared_mutex>
#include <unordered_map>
namespace logtail {
class TargetAllocator {
public:
    TargetAllocator(const TargetAllocator&) = delete;
    TargetAllocator& operator=(const TargetAllocator&) = delete;
    static TargetAllocator* GetInstance() {
        static TargetAllocator instance;
        return &instance;
    }
    struct TargetGroup {
        using LabelSet = std::unordered_map<std::string, std::string>;
        std::vector<std::string> targets;
        LabelSet labels;
        std::string source;
    };
    const Json::Value& GetConfig() const { return config; }
    const std::string& GetJobName() const { return jobName; }
    const std::vector<std::shared_ptr<TargetGroup>>& GetTargets() const {
        return targetGroups;
    }
    bool Init(const Json::Value& config);
    bool Start();
private:
    TargetAllocator() = default;
    ~TargetAllocator() = default;
    const int refeshIntervalSeconds = 5;
    Json::Value config;
    std::string jobName;
    std::vector<std::shared_ptr<TargetGroup>> targetGroups;
    static size_t WriteCallback(char* contents, size_t size, size_t nmemb, std::string* userp);
    void FetchHttpData(const std::string& url, std::string& readBuffer);
    std::vector<std::shared_ptr<TargetGroup>> ParseTargetGroups(const std::string& response,
                                                                const Json::Value& httpSDConfig);
    void Refresh(const Json::Value& httpSDConfig, const std::string& jobName);
};
} // namespace logtail