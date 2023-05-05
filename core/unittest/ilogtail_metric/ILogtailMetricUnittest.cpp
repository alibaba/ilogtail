#include "unittest/Unittest.h"
#include <fstream>
#include <json/json.h>
#include "ILogtailMetric.h"
#include <list>
#include "util.h"

namespace logtail {

class ILogtailMetricUnittest : public ::testing::Test {
public:
    void TestCreateMetric();
    void TestDeleteMetric();
};

APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateMetric, 0);
APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestDeleteMetric, 0);

void ILogtailMetricUnittest::TestCreateMetric() {
    // create
    ILogtailMetric::MetricGroup* group = ILogtailMetric::GetInstance()->CreateMetric("test", "test");
    ILogtailMetric::GroupInfo meta = {project: "project_test", logstore: "logstore_test", configName: "configName_test"};
    group->meta = &meta;

    ILogtailMetric::Metric metric= {};
    metric.name = "metric_1";
    metric.value = 0;
    metric.unit = "ms";
    metric.description = "taiye_test";
    group->metrics.push_back(&metric);

    metric.value += 11;

    std::list<ILogtailMetric::Metric*>::iterator iter;	// 迭代器
	for(iter=group->metrics.begin();iter!=group->metrics.end();iter++)
	{
        ILogtailMetric::Metric* pMetric = *iter;
        LOG_INFO(sLogger,("metric_value", pMetric->value));
	}

    LOG_INFO(sLogger,
            ("project", group->meta->project)("logstore", group->meta->logstore)("configName", group->meta->configName));
    APSARA_TEST_EQUAL(ILogtailMetric::GetInstance()->GetInstanceMetrics().size(), 1);    
}

void ILogtailMetricUnittest::TestDeleteMetric() {
    
}
}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}