/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "prometheus/ScrapeJob.h"
#include "sdk/Common.h"

namespace logtail {

class PrometheusInputRunner {
public:
    static PrometheusInputRunner* GetInstance() {
        static PrometheusInputRunner instance;
        return &instance;
    }

    void UpdateScrapeInput(const std::string& inputName, std::unique_ptr<ScrapeJob> scrapeJob);
    void RemoveScrapeInput(const std::string& inputName);

    void Start();
    void Stop();
    bool HasRegisteredPlugin();

private:
    PrometheusInputRunner();
    PrometheusInputRunner(const PrometheusInputRunner&) = delete;
    PrometheusInputRunner(PrometheusInputRunner&&) = delete;
    PrometheusInputRunner& operator=(const PrometheusInputRunner&) = delete;
    PrometheusInputRunner& operator=(PrometheusInputRunner&&) = delete;

    std::unordered_map<std::string, std::string> mPrometheusInputsMap;

    std::unique_ptr<sdk::HTTPClient> mClient;

    std::string mOperatorHost;
    int32_t mOperatorPort;
    std::string mPodName;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;

#endif
};

} // namespace logtail
