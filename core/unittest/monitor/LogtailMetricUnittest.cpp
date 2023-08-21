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
    labels.emplace_back(std::make_pair<std::string, std::string>("project","project1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("logstore","logstore1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));

    MetricsRef fileMetric;
    fileMetric.Init(labels);
    APSARA_TEST_EQUAL(fileMetric.Get()->GetLabels().size(), 3);  


    CounterPtr fileCounter = fileMetric.Get()->CreateCounter("filed1");
    fileCounter->Add((uint64_t)111);
    APSARA_TEST_EQUAL(fileCounter->GetValue(), 111);  
    

    {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("project","project1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("logstore","logstore1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));

        MetricsRef fileMetric2;
        fileMetric2.Init(labels);
        CounterPtr fileCounter2 = fileMetric2.Get()->CreateCounter("filed2");
        fileCounter2->Add((uint64_t)222);
    }
    
    {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("project","project1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("logstore","logstore1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));
        MetricsRef fileMetric3;
        fileMetric3.Init(labels);
        CounterPtr fileCounter3 = fileMetric3.Get()->CreateCounter("filed3");
        fileCounter3->Add((uint64_t)333);
    }

    ReadMetrics::GetInstance()->UpdateMetrics();
    
    Metrics* head = ReadMetrics::GetInstance()->GetHead();
    int count = 0;

    while(head) {
        head = head->GetNext();
        count ++;
    } 
    APSARA_TEST_EQUAL(count, 1);  

    MetricExportor::GetInstance()->PushMetrics();
}

void PushMetrics() {
    for (int i = 0; i < 10; i ++) {
        MetricExportor::GetInstance()->PushMetrics();
    }
}

void createMetrics(int count) {
    for (int i = 0; i < count; i ++) {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("num", std::to_string(i)));
        labels.emplace_back(std::make_pair<std::string, std::string>("count", std::to_string(count)));
        labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-beijing"));
        MetricsRef fileMetric;
        fileMetric.Init(labels);
        CounterPtr fileCounter = fileMetric.Get()->CreateCounter("filed1");
        fileCounter->Add((uint64_t)111);
    }
}

void UpdateMetrics() {
    for (int i = 0; i < 10; i ++) {
        ReadMetrics::GetInstance()->UpdateMetrics();
        {
            Metrics* head = WriteMetrics::GetInstance()->GetHead();
            int count = 0;
            while(head) {
                if (!head->IsDeleted()) {
                    count ++;
                }
                head = head->GetNext();
            }    
            LOG_INFO(sLogger, ("WriteCount", count));
        }
        {
            Metrics* head = ReadMetrics::GetInstance()->GetHead();
            int count = 0;
            while(head) {
                head = head->GetNext();
                count ++;
            }    
            LOG_INFO(sLogger, ("ReadCount", count));
        }
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
    
    Metrics* head = WriteMetrics::GetInstance()->GetHead();
    Metrics* tmp = head;
    int count = 0;
    while(tmp != nullptr) {
        for (auto& it: tmp->GetLabels()) {
            LOG_INFO(sLogger, ("key", it.first)("value", it.second));
        }
        tmp = tmp->GetNext();
        count ++;
    }    
    // first test left 1, 1 + (1 + 2 + 3 + 4)
    APSARA_TEST_EQUAL(count, 11);  
    LOG_INFO(sLogger, ("Count", count));
    
    // UpdateMetrics multi time
    for (int i = 0; i < 10; i ++) {
        ReadMetrics::GetInstance()->UpdateMetrics();
    }

    {
        Metrics* head = WriteMetrics::GetInstance()->GetHead();
        Metrics* tmp = head;
        int count = 0;
        while(tmp != nullptr) {
            for (auto& it: tmp->GetLabels()) {
                LOG_INFO(sLogger, ("key", it.first)("value", it.second));
            }
            tmp = tmp->GetNext();
            count ++;
        }  
        APSARA_TEST_EQUAL(count, 0);  
    }

}



void ILogtailMetricUnittest::TestCreateAndDeleteMetricMultiThread() {
    
    std::thread t1(createMetrics, 5);
    std::thread t2(createMetrics, 6);

    // create one in main thread
    std::vector<std::pair<std::string, std::string>> labels;
    labels.emplace_back(std::make_pair<std::string, std::string>("project","project1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("logstore","logstore1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));

    MetricsRef fileMetric;
    fileMetric.Init(labels);
    CounterPtr fileCounter = fileMetric.Get()->CreateCounter("filed2");
    fileCounter->Add((uint64_t)1);

    // UpdateMetrics multi time while createMetrics
    std::thread tUpdate(PushMetrics);

    std::thread t3(createMetrics, 7);
    std::thread t4(createMetrics, 8);
    
    t1.join();
    t2.join();
    tUpdate.join();
    t3.join();
    t4.join();
    

    // final UpdateMetrics after createMetrics
    ReadMetrics::GetInstance()->UpdateMetrics();
    {
        Metrics* head = WriteMetrics::GetInstance()->GetHead();
        int count = 0;
        while(head) {
            if (!head->IsDeleted()) {
                count ++;
            }
            head = head->GetNext();
        }    
        APSARA_TEST_EQUAL(count, 1);  
        LOG_INFO(sLogger, ("FinalWriteCount", count));

    }
    {
        Metrics* head = ReadMetrics::GetInstance()->GetHead();
        int count = 0;
        while(head) {
            head = head->GetNext();
            count ++;
        }    
        // only one left
        APSARA_TEST_EQUAL(count, 1);  

        head = ReadMetrics::GetInstance()->GetHead();
        fileCounter->Add((uint64_t)2);
        // after UpdateMetrics, couter will be reset
        APSARA_TEST_EQUAL(fileCounter->GetValue(), 2);  
        APSARA_TEST_EQUAL(head->GetLabels().size(), 3);  

        LOG_INFO(sLogger, ("FinalReadCount", count));
    }
    MetricExportor::GetInstance()->PushMetrics();
}


}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}