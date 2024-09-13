
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
    selfMonitor->Init("pod-xxx", "operator-service");
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {prometheus::PROM_SUBSCRIBE_TARGETS, MetricType::METRIC_TYPE_INT_GAUGE},
        {prometheus::PROM_SUBSCRIBE_TOTAL, MetricType::METRIC_TYPE_COUNTER},
    };
    selfMonitor->InitMetricManager("test-key", testMetricKeys);

    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap.count("test-key"));
    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap["test-key"].count(prometheus::PROM_SUBSCRIBE_TARGETS));
    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap["test-key"].count(prometheus::PROM_SUBSCRIBE_TOTAL));
}

void PromSelfMonitorUnittest::TestCounterAdd() {
    auto selfMonitor = std::make_shared<PromSelfMonitor>();
    selfMonitor->Init("pod-xxx", "operator-service");
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {prometheus::PROM_SUBSCRIBE_TOTAL, MetricType::METRIC_TYPE_COUNTER},
    };
    selfMonitor->InitMetricManager("test-key", testMetricKeys);
    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap.count("test-key"));

    auto metricLabels = std::map<std::string, std::string>({{"test-label", "test-value"}});
    auto metricVectorLabels = MetricLabels(metricLabels.begin(), metricLabels.end());
    selfMonitor->CounterAdd("test-key", prometheus::PROM_SUBSCRIBE_TOTAL, metricLabels, 999);

    // check result
    auto metricManager = selfMonitor->mPromMetricsMap["test-key"][prometheus::PROM_SUBSCRIBE_TOTAL];
    auto recordRef = metricManager->GetOrCreateReentrantMetricsRecordRef(metricVectorLabels);
    auto metric = recordRef->GetCounter(prometheus::PROM_SUBSCRIBE_TOTAL);
    APSARA_TEST_EQUAL("prom_subscribe_total", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->CounterAdd("test-key", prometheus::PROM_SUBSCRIBE_TOTAL, metricLabels);
    APSARA_TEST_EQUAL(1000ULL, metric->GetValue());

    // modify labels
    metricLabels["test-label"] = "test-value2";
    metricVectorLabels = MetricLabels(metricLabels.begin(), metricLabels.end());
    selfMonitor->CounterAdd("test-key", prometheus::PROM_SUBSCRIBE_TOTAL, metricLabels, 987);

    // check result
    APSARA_TEST_EQUAL(1000ULL, metric->GetValue());
    metricManager = selfMonitor->mPromMetricsMap["test-key"][prometheus::PROM_SUBSCRIBE_TOTAL];
    recordRef = metricManager->GetOrCreateReentrantMetricsRecordRef(metricVectorLabels);
    metric = recordRef->GetCounter(prometheus::PROM_SUBSCRIBE_TOTAL);
    APSARA_TEST_EQUAL(987ULL, metric->GetValue());
}

void PromSelfMonitorUnittest::TestIntGaugeSet() {
    auto selfMonitor = std::make_shared<PromSelfMonitor>();
    selfMonitor->Init("pod-xxx", "operator-service");
    std::unordered_map<std::string, MetricType> testMetricKeys = {
        {prometheus::PROM_SUBSCRIBE_TARGETS, MetricType::METRIC_TYPE_INT_GAUGE},
    };
    selfMonitor->InitMetricManager("test-key", testMetricKeys);
    APSARA_TEST_TRUE(selfMonitor->mPromMetricsMap.count("test-key"));

    auto metricLabels = std::map<std::string, std::string>({{"test-label", "test-value"}});
    auto metricVectorLabels = MetricLabels(metricLabels.begin(), metricLabels.end());
    selfMonitor->IntGaugeSet("test-key", prometheus::PROM_SUBSCRIBE_TARGETS, metricLabels, 999);

    // check result
    auto metricManager = selfMonitor->mPromMetricsMap["test-key"][prometheus::PROM_SUBSCRIBE_TARGETS];
    auto recordRef = metricManager->GetOrCreateReentrantMetricsRecordRef(metricVectorLabels);
    auto metric = recordRef->GetIntGauge(prometheus::PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL("prom_subscribe_targets", metric->GetName());
    APSARA_TEST_EQUAL(999ULL, metric->GetValue());
    selfMonitor->IntGaugeSet("test-key", prometheus::PROM_SUBSCRIBE_TARGETS, metricLabels, 0);
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());

    // modify labels
    metricLabels["test-label"] = "test-value2";
    metricVectorLabels = MetricLabels(metricLabels.begin(), metricLabels.end());
    selfMonitor->IntGaugeSet("test-key", prometheus::PROM_SUBSCRIBE_TARGETS, metricLabels, 987);

    // check result
    APSARA_TEST_EQUAL(0ULL, metric->GetValue());
    metricManager = selfMonitor->mPromMetricsMap["test-key"][prometheus::PROM_SUBSCRIBE_TARGETS];
    recordRef = metricManager->GetOrCreateReentrantMetricsRecordRef(metricVectorLabels);
    metric = recordRef->GetIntGauge(prometheus::PROM_SUBSCRIBE_TARGETS);
    APSARA_TEST_EQUAL(987ULL, metric->GetValue());
}


UNIT_TEST_CASE(PromSelfMonitorUnittest, TestInitMetricManager)
UNIT_TEST_CASE(PromSelfMonitorUnittest, TestCounterAdd)
UNIT_TEST_CASE(PromSelfMonitorUnittest, TestIntGaugeSet)

} // namespace logtail

UNIT_TEST_MAIN
