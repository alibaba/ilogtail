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
};

APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateMetric, 0);

void ILogtailMetricUnittest::TestCreateMetric() {
    // create
    //PluginMetric* pluginMetric = new PluginMetric();
    std::vector<std::pair<std::string, std::string>> labels;
    labels.push_back(std::make_pair<std::string, std::string>("project","project1"));
    labels.push_back(std::make_pair<std::string, std::string>("logstore","logstore1"));

    Metrics* fileMetric = WriteMetrics::GetInstance()->CreateMetrics(labels);
    LOG_INFO(sLogger, ("labelSize", fileMetric->GetLabels().size()));
    APSARA_TEST_EQUAL(fileMetric->GetLabels().size(), 2);  

    Metrics* fileMetric2 = WriteMetrics::GetInstance()->CreateMetrics(labels);
    LOG_INFO(sLogger, ("labelSize", fileMetric2->GetLabels().size()));
    APSARA_TEST_EQUAL(fileMetric2->GetLabels().size(), 2);  

    Metrics* fileMetric3 = WriteMetrics::GetInstance()->CreateMetrics(labels);
    LOG_INFO(sLogger, ("labelSize", fileMetric3->GetLabels().size()));
    APSARA_TEST_EQUAL(fileMetric3->GetLabels().size(), 2);  


    Counter* fileCounter = fileMetric->CreateCounter("filed1");
    fileCounter->Add((uint64_t)111);
    LOG_INFO(sLogger, ("value", fileCounter->GetValue())("time", fileCounter->GetTimestamp()));

    Counter* fileCounter2 = fileMetric->CreateCounter("filed2");
    fileCounter2->Add((uint64_t)222);
    LOG_INFO(sLogger, ("value", fileCounter2->GetValue())("time", fileCounter2->GetTimestamp()));


    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric2);
    // delete first element
    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric);
    ReadMetrics::GetInstance()->UpdateMetrics();
    
    Metrics* head = ReadMetrics::GetInstance()->mHead;
    while(head) {
        LOG_INFO(sLogger, ("ReadMetrics", head->GetLabels().size()));
        head = head->next;
    }    
}

}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}