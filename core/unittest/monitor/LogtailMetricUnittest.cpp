#include "unittest/Unittest.h"
#include <fstream>
#include <json/json.h>
#include <list>
#include <atomic>
#include <thread>
#include "util.h"
#include "LogtailMetric.h"
#include "MetricExportor.h"
#include "MetricConstants.h"

namespace logtail {

class ILogtailMetricUnittest : public ::testing::Test {
public:
    void TestCreateMetric();
    void TestCreateMetricMultiThread();
    void TestCreateAndDeleteMetricMultiThread();
};

APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateMetric, 0);
APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateMetricMultiThread, 0);
APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateAndDeleteMetricMultiThread, 0);


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


    CounterPtr fileCounter = fileMetric->CreateCounter("filed1");
    fileCounter->Add((uint64_t)111);
    LOG_INFO(sLogger, ("value", fileCounter->GetValue())("time", fileCounter->GetTimestamp()));

    CounterPtr fileCounter2 = fileMetric->CreateCounter("filed2");
    fileCounter2->Add((uint64_t)222);
    LOG_INFO(sLogger, ("value", fileCounter2->GetValue())("time", fileCounter2->GetTimestamp()));

    CounterPtr fileCounter3 = fileMetric3->CreateCounter("filed2");
    fileCounter3->Add((uint64_t)222);
    LOG_INFO(sLogger, ("value", fileCounter3->GetValue())("time", fileCounter3->GetTimestamp()));


    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric2);
    // delete first element
    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric);
    ReadMetrics::GetInstance()->UpdateMetrics();
    
    Metrics* head = ReadMetrics::GetInstance()->mHead;
    while(head) {
        LOG_INFO(sLogger, ("ReadMetrics", head->GetLabels().size()));
        head = head->next;
    }    
    MetricExportor::GetInstance()->PushMetrics();
}

void createMetrics(int count) {
    for (int i = 0; i < count; i ++) {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.push_back(std::make_pair<std::string, std::string>("num", std::to_string(i)));
        labels.push_back(std::make_pair<std::string, std::string>("count", std::to_string(count)));
        Metrics* fileMetric = WriteMetrics::GetInstance()->CreateMetrics(labels);
        CounterPtr fileCounter = fileMetric->CreateCounter("filed1");
        fileCounter->Add((uint64_t)111);
        //LOG_INFO(sLogger, ("num", i )("count", count));
    }
}

void createAndDeleteMetrics(int count) {
    for (int i = 0; i < count; i ++) {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.push_back(std::make_pair<std::string, std::string>("num", std::to_string(i)));
        labels.push_back(std::make_pair<std::string, std::string>("count", std::to_string(count)));
        Metrics* fileMetric = WriteMetrics::GetInstance()->CreateMetrics(labels);
        CounterPtr fileCounter = fileMetric->CreateCounter("filed1");
        fileCounter->Add((uint64_t)111);
        //LOG_INFO(sLogger, ("num", i )("count", count));
        WriteMetrics::GetInstance()->DestroyMetrics(fileMetric);
    }
}

void UpdateMetrics() {
    ReadMetrics::GetInstance()->UpdateMetrics();
    Metrics* head = ReadMetrics::GetInstance()->mHead;
    while(head) {
        std::vector<std::pair<std::string, std::string>> labels = head->GetLabels();
        for (std::vector<std::pair<std::string, std::string>>::iterator it = labels.begin(); it != labels.end(); ++it) {
            std::pair<std::string, std::string> pair = *it;
            //LOG_INFO(sLogger, ("key", pair.first)("value", pair.second));
        }
        head = head->next;
    }    
}

void ILogtailMetricUnittest::TestCreateMetricMultiThread() {
    
    std::thread t1(createMetrics, 1);
    std::thread t2(createMetrics, 2);
    std::thread t3(createMetrics, 3);
    std::thread t4(createMetrics, 4);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    Metrics* head = WriteMetrics::GetInstance()->mHead;
    int count = 0;
    while(head) {
        std::vector<std::pair<std::string, std::string>> labels = head->GetLabels();
        for (std::vector<std::pair<std::string, std::string>>::iterator it = labels.begin(); it != labels.end(); ++it) {
            std::pair<std::string, std::string> pair = *it;
            //LOG_INFO(sLogger, ("key", pair.first)("value", pair.second));
        }
        head = head->next;
        count ++;
    }    
    LOG_INFO(sLogger, ("Count", count));
}



void ILogtailMetricUnittest::TestCreateAndDeleteMetricMultiThread() {
    
    std::thread t1(createAndDeleteMetrics, 5);
    std::thread t2(createAndDeleteMetrics, 6);

    std::thread tUpdate(UpdateMetrics);

    std::thread t3(createAndDeleteMetrics, 7);
    std::thread t4(createAndDeleteMetrics, 8);
    
    t1.join();
    t2.join();
    tUpdate.join();
    t3.join();
    t4.join();

    {
        Metrics* head = WriteMetrics::GetInstance()->mHead;
        int count = 0;
        while(head) {
            if (!head-> IsDeleted()) {
                count ++;
            }
            head = head->next;
        }    
        LOG_INFO(sLogger, ("WriteCount", count));

    }
    {
        Metrics* head = ReadMetrics::GetInstance()->mHead;
        int count = 0;
        while(head) {
            head = head->next;
            count ++;
        }    
        LOG_INFO(sLogger, ("ReadCount", count));

    }

    UpdateMetrics();
    {
        Metrics* head = WriteMetrics::GetInstance()->mHead;
        int count = 0;
        while(head) {
            if (!head-> IsDeleted()) {
                count ++;
            }
            head = head->next;
        }    
        LOG_INFO(sLogger, ("FinalWriteCount", count));

    }
    {
        Metrics* head = ReadMetrics::GetInstance()->mHead;
        int count = 0;
        while(head) {
            head = head->next;
            count ++;
        }    
        LOG_INFO(sLogger, ("FinalReadCount", count));

    }
}


}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}