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

    void Start();
    void Stop();
    void HasRegisteredPlugin();

    // void HoldOn();
    // void Reload();

private:
    std::unordered_map<std::string, std::set<ScrapeJob>> scrapeInputsMap;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
#endif
};

} // namespace logtail
