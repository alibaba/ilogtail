
#include "monitor/metric_constants/MetricConstants.h"
#include "prometheus/Constants.h"
#include "prometheus/PromSelfMonitor.h"
#include "unittest/Unittest.h"
using namespace std;

namespace logtail {
class PromSelfMonitorUnittest : public ::testing::Test {
public:
    void TestCounterAdd();
    void TestIntGaugeSet();
    void TestCurlCode();
};

void PromSelfMonitorUnittest::TestCounterAdd() {
    auto selfMonitor = std::make_shared<PromSelfMonitorUnsafe>();
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {METRIC_PLUGIN_PROM_SUBSCRIBE_TOTAL, MetricType::METRIC_TYPE_COUNTER},
    };
    selfMonitor->InitMetricManager(testMetricKeys, MetricLabels{});

    selfMonitor->AddCounter(METRIC_PLUGIN_PROM_SUBSCRIBE_TOTAL, 200, 999);

    // check result
    auto metric = selfMonitor->mPromStatusMap["200"]->GetCounter(METRIC_PLUGIN_PROM_SUBSCRIBE_TOTAL);
    APSARA_TEST_EQUAL("prom_subscribe_total", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->AddCounter(METRIC_PLUGIN_PROM_SUBSCRIBE_TOTAL, 200);
    APSARA_TEST_EQUAL(1000ULL, metric->GetValue());
}

void PromSelfMonitorUnittest::TestIntGaugeSet() {
    auto selfMonitor = std::make_shared<PromSelfMonitorUnsafe>();
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, MetricType::METRIC_TYPE_INT_GAUGE},
    };
    selfMonitor->InitMetricManager(testMetricKeys, MetricLabels{});

    auto metricLabels = std::map<std::string, std::string>({{"test-label", "test-value"}});
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 200, 999);

    // check result
    auto metric = selfMonitor->mPromStatusMap["200"]->GetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 200, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());
}

void PromSelfMonitorUnittest::TestCurlCode() {
    auto selfMonitor = std::make_shared<PromSelfMonitorUnsafe>();
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, MetricType::METRIC_TYPE_INT_GAUGE},
    };
    selfMonitor->InitMetricManager(testMetricKeys, MetricLabels{});

    // 200
    auto metricLabels = std::map<std::string, std::string>({{"test-label", "test-value"}});
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 200, 999);
    // check result
    auto metric = selfMonitor->mPromStatusMap["200"]->GetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 200, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());

    // 301
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 301, 999);
    // check result
    metric = selfMonitor->mPromStatusMap["301"]->GetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 301, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());

    // 678
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 678, 999);
    // check result
    metric = selfMonitor->mPromStatusMap["other"]->GetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 678, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());

    // 0
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 0, 999);
    // check result
    metric = selfMonitor->mPromStatusMap["OK"]->GetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 0, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());

    // 35
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 35, 999);
    // check result
    metric = selfMonitor->mPromStatusMap["ERR_SSL_CONN_ERR"]->GetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 35, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());

    // 88
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 88, 999);
    // check result
    metric = selfMonitor->mPromStatusMap["ERR_UNKNOWN"]->GetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->SetIntGauge(METRIC_PLUGIN_PROM_SUBSCRIBE_TARGETS, 88, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());
}


UNIT_TEST_CASE(PromSelfMonitorUnittest, TestCounterAdd)
UNIT_TEST_CASE(PromSelfMonitorUnittest, TestIntGaugeSet)
UNIT_TEST_CASE(PromSelfMonitorUnittest, TestCurlCode)

} // namespace logtail

UNIT_TEST_MAIN
