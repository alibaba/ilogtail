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
    labels.push_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));

    Metrics* fileMetric = WriteMetrics::GetInstance()->CreateMetrics(labels);
    APSARA_TEST_EQUAL(fileMetric->GetLabels().size(), 3);  

    Metrics* fileMetric2 = WriteMetrics::GetInstance()->CreateMetrics(labels);

    Metrics* fileMetric3 = WriteMetrics::GetInstance()->CreateMetrics(labels);


    CounterPtr fileCounter = fileMetric->CreateCounter("filed1");
    fileCounter->Add((uint64_t)111);
    APSARA_TEST_EQUAL(fileCounter->GetValue(), 111);  

    CounterPtr fileCounter2 = fileMetric->CreateCounter("filed2");
    fileCounter2->Add((uint64_t)222);
    
    CounterPtr fileCounter3 = fileMetric3->CreateCounter("filed3");
    fileCounter3->Add((uint64_t)333);


    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric2);
    // delete first element
    WriteMetrics::GetInstance()->DestroyMetrics(fileMetric3);
    ReadMetrics::GetInstance()->UpdateMetrics();
    
    Metrics* head = ReadMetrics::GetInstance()->mHead;
    int count = 0;
    while(head) {
        head = head->next;
        count ++;
    } 
    APSARA_TEST_EQUAL(count, 1);  
    MetricExportor::GetInstance()->PushMetrics();
}

void createMetrics(int count) {
    for (int i = 0; i < count; i ++) {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.push_back(std::make_pair<std::string, std::string>("num", std::to_string(i)));
        labels.push_back(std::make_pair<std::string, std::string>("count", std::to_string(count)));
        labels.push_back(std::make_pair<std::string, std::string>("region","cn-beijing"));
        Metrics* fileMetric = WriteMetrics::GetInstance()->CreateMetrics(labels);
        CounterPtr fileCounter = fileMetric->CreateCounter("filed1");
        fileCounter->Add((uint64_t)111);
    }
}

void createAndDeleteMetrics(int count) {
    for (int i = 0; i < count; i ++) {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.push_back(std::make_pair<std::string, std::string>("num", std::to_string(i)));
        labels.push_back(std::make_pair<std::string, std::string>("count", std::to_string(count)));
        labels.push_back(std::make_pair<std::string, std::string>("region","cn-shanghai"));
        Metrics* fileMetric = WriteMetrics::GetInstance()->CreateMetrics(labels);
        CounterPtr fileCounter = fileMetric->CreateCounter("filed1");
        fileCounter->Add((uint64_t)111);
        //WriteMetrics::GetInstance()->DestroyMetrics(fileMetric);
    }
}

void UpdateMetrics() {
    ReadMetrics::GetInstance()->UpdateMetrics();
    Metrics* head = ReadMetrics::GetInstance()->mHead;
    while(head) {
        std::vector<std::pair<std::string, std::string>> labels = head->GetLabels();
        for (std::vector<std::pair<std::string, std::string>>::iterator it = labels.begin(); it != labels.end(); ++it) {
            std::pair<std::string, std::string> pair = *it;
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
    // first test left 1, 1 + (1 + 2 + 3 + 4)
    APSARA_TEST_EQUAL(count, 11);  
    LOG_INFO(sLogger, ("Count", count));
    MetricExportor::GetInstance()->PushMetrics();
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
        APSARA_TEST_EQUAL(count, 11);  
        LOG_INFO(sLogger, ("FinalWriteCount", count));

    }
    {
        Metrics* head = ReadMetrics::GetInstance()->mHead;
        int count = 0;
        while(head) {
            head = head->next;
            count ++;
        }    
        APSARA_TEST_EQUAL(count, 11);  
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