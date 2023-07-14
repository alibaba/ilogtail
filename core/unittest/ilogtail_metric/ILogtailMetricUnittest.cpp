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
    std::vector<std::pair<std::string, std::string>> labels;
    labels.push_back(std::make_pair<std::string, std::string>("project","project1"));
    labels.push_back(std::make_pair<std::string, std::string>("logstore","logstore1"));

    Metrics* fileMetric = WriteMetrics::GetInstance()->CreateMetrics(labels);
    LOG_INFO(sLogger, ("labelSize", fileMetric->mLabels.size()));
    APSARA_TEST_EQUAL(fileMetric->mLabels.size(), 2);  

    Metrics* fileMetric2 = WriteMetrics::GetInstance()->CreateMetrics(labels);
    LOG_INFO(sLogger, ("labelSize", fileMetric2->mLabels.size()));
    APSARA_TEST_EQUAL(fileMetric2->mLabels.size(), 2);  

    Metrics* fileMetric3 = WriteMetrics::GetInstance()->CreateMetrics(labels);
    LOG_INFO(sLogger, ("labelSize", fileMetric3->mLabels.size()));
    APSARA_TEST_EQUAL(fileMetric3->mLabels.size(), 2);  


    Counter* fileCounter = fileMetric->CreateCounter("filed1");
    fileCounter->Add((uint64_t)111);
    LOG_INFO(sLogger, ("value", fileCounter->mVal)("time", fileCounter->mTimestamp));

    Counter* fileCounter2 = fileMetric->CreateCounter("filed2");
    fileCounter2->Add((uint64_t)222);
    LOG_INFO(sLogger, ("value", fileCounter2->mVal)("time", fileCounter2->mTimestamp));


    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric2);
    // delete first element
    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric);
    ReadMetrics::GetInstance()->UpdateMetrics();
    
    Metrics* head = ReadMetrics::GetInstance()->mHead;
    while(head) {
        LOG_INFO(sLogger, ("ReadMetrics", head->mLabels.size()));
        head = head->next;
    }    
}

void ILogtailMetricUnittest::TestDeleteMetric() {
    
}
}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}