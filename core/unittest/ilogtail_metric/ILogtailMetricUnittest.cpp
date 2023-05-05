#include "unittest/Unittest.h"
#include <fstream>
#include <json/json.h>
#include "ILogtailMetric.h"
#include "util.h"

namespace logtail {

// global var
const char* gLineCountJsonFile = "logtail_line_count_snapshot.json";
const char* gIntegrityJsonFile = "logtail_integrity_snapshot.json";

class ILogtailMetricUnittest : public ::testing::Test {
public:
    void TestCreateMetric();
    void TestDeleteMetric();
};

APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateMetric, 0);
APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestDeleteMetric, 0);

void ILogtailMetricUnittest::TestCreateMetric() {
    // create
    ILogtailMetric::GetInstance()->CreateMetric("test", "test");
    APSARA_TEST_EQUAL(ILogtailMetric::GetInstance()->GetInstanceMetrics().size(), 1);    
}

void ILogtailMetricUnittest::TestDeleteMetric() {
    
}
}