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
    PrometheusInputRunner();

    static PrometheusInputRunner* GetInstance() {
        static auto runner = new PrometheusInputRunner;
        return runner;
    }

    void UpdateScrapeInput(const std::string& inputName, const std::vector<ScrapeJob>& jobs);
    void RemoveScrapeInput(const std::string& inputName);

    void Start();
    void Stop();
    bool HasRegisteredPlugin();

    // void HoldOn();
    // void Reload();

private:
    std::unordered_map<std::string, std::set<ScrapeJob>> scrapeInputsMap;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail
