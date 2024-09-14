
#include "monitor/MetricConstants.h"
#include "prometheus/Constants.h"
#include "prometheus/PromSelfMonitor.h"
#include "unittest/Unittest.h"
using namespace std;

namespace logtail {
class PromSelfMonitorUnittest : public ::testing::Test {
public:
    void TestInitMetricManager();
    void TestCounterAdd();
    void TestIntGaugeSet();
};

void PromSelfMonitorUnittest::TestInitMetricManager() {
    auto selfMonitor = std::make_shared<PromSelfMonitor>();
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {PROM_SUBSCRIBE_TARGETS, MetricType::METRIC_TYPE_INT_GAUGE},
        {PROM_SUBSCRIBE_TOTAL, MetricType::METRIC_TYPE_COUNTER},
    };
    selfMonitor->InitMetricManager("test-key", testMetricKeys, MetricLabels{});

    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap.count("test-key"));
}

void PromSelfMonitorUnittest::TestCounterAdd() {
    auto selfMonitor = std::make_shared<PromSelfMonitor>();
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {PROM_SUBSCRIBE_TOTAL, MetricType::METRIC_TYPE_COUNTER},
    };
    selfMonitor->InitMetricManager("test-key", testMetricKeys, MetricLabels{});
    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap.count("test-key"));

    selfMonitor->CounterAdd("test-key", PROM_SUBSCRIBE_TOTAL, 200, 999);

    // check result
    auto metric = selfMonitor->mPromStatusMap["test-key"][200]->GetCounter(PROM_SUBSCRIBE_TOTAL);
    APSARA_TEST_EQUAL("prom_subscribe_total", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->CounterAdd("test-key", PROM_SUBSCRIBE_TOTAL, 200);
    APSARA_TEST_EQUAL(1000ULL, metric->GetValue());
}

void PromSelfMonitorUnittest::TestIntGaugeSet() {
    auto selfMonitor = std::make_shared<PromSelfMonitor>();
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {PROM_SUBSCRIBE_TARGETS, MetricType::METRIC_TYPE_INT_GAUGE},
    };
    selfMonitor->InitMetricManager("test-key", testMetricKeys, MetricLabels{});
    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap.count("test-key"));

    auto metricLabels = std::map<std::string, std::string>({{"test-label", "test-value"}});
    selfMonitor->IntGaugeSet("test-key", PROM_SUBSCRIBE_TARGETS, 200, 999);

    // check result
    auto metric = selfMonitor->mPromStatusMap["test-key"][200]->GetIntGauge(PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->IntGaugeSet("test-key", PROM_SUBSCRIBE_TARGETS, 200, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());
}


UNIT_TEST_CASE(PromSelfMonitorUnittest, TestInitMetricManager)
UNIT_TEST_CASE(PromSelfMonitorUnittest, TestCounterAdd)
UNIT_TEST_CASE(PromSelfMonitorUnittest, TestIntGaugeSet)

} // namespace logtail

UNIT_TEST_MAIN
