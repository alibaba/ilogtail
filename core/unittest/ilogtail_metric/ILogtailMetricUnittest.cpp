#include "unittest/Unittest.h"
#include <fstream>
#include <json/json.h>
#include <list>
#include <atomic>
#include "util.h"
#include "ILogtailMetric.h"
#include "MetricConstants.h"

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
    //PluginMetric* pluginMetric = new PluginMetric();
    PipelineMetric* fileMetric = ILogtailMetric::GetInstance()->createFileMetric("configTest", "pluginTest","fileTest");
    LOG_INFO(sLogger, ("labelSize", fileMetric->mLabels.size()));
    APSARA_TEST_EQUAL(fileMetric->mLabels.size(), 3);  



    BaseMetric* base = ILogtailMetric::GetInstance()->getBaseMetric(fileMetric, METRIC_FILE_READ_COUNT);
    base->baseMetricAdd((uint64_t)123);
    LOG_INFO(sLogger, ("value", base->getMetricObj()->val));
    //APSARA_TEST_EQUAL(base->getMetricObj()->val, 123);  
}

void ILogtailMetricUnittest::TestDeleteMetric() {
    
}
}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}