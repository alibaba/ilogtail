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

#include <string>
#include <vector>

#include "Scraper.h"
#include "common/Lock.h"
#include "common/Thread.h"
#include "input/InputPrometheus.h"
#include "sdk/Common.h"

namespace logtail {

class PrometheusInputRunner {
public:
    PrometheusInputRunner(const PrometheusInputRunner&) = delete;
    PrometheusInputRunner& operator=(const PrometheusInputRunner&) = delete;

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
    PrometheusInputRunner() = default;
    ~PrometheusInputRunner() = default;

    std::unordered_map<std::string, std::string> mPrometheusInputsMap;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail
