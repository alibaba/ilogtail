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
    SubMetric* fileMetric = ILogtailMetric::GetInstance()->createFileSubMetric(NULL, "test");
    LOG_INFO(sLogger,
            ("labelSize", fileMetric->getLabels().size()));
    //APSARA_TEST_EQUAL(ILogtailMetric::GetInstance()->GetInstanceMetrics().size(), 1);    
}

void ILogtailMetricUnittest::TestDeleteMetric() {
    
}
}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}