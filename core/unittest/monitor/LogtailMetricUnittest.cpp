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


static std::atomic_bool running(true);


class ILogtailMetricUnittest : public ::testing::Test {
public:
    void TestCreateMetric();
    void TestCreateMetricMultiThread();
    void TestCreateAndDeleteMetricMultiThread();
};

APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateMetric, 0);
APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateMetricMultiThread, 1);
APSARA_UNIT_TEST_CASE(ILogtailMetricUnittest, TestCreateAndDeleteMetricMultiThread, 2);


void ILogtailMetricUnittest::TestCreateMetric() {
    // create
    //PluginMetric* pluginMetric = new PluginMetric();
    
    std::vector<std::pair<std::string, std::string>> labels;
    labels.emplace_back(std::make_pair<std::string, std::string>("project","project1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("logstore","logstore1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));

    MetricsRef fileMetric;
    fileMetric.Init(std::move(labels));
    APSARA_TEST_EQUAL(fileMetric.Get()->GetLabels().size(), 3);  


    MetricPtr fileCounter = fileMetric.Get()->CreateCounter("filed1");
    fileCounter->SetValue((uint64_t)111);
    APSARA_TEST_EQUAL(fileCounter->GetValue(), 111);  
    

    {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("project","project1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("logstore","logstore1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));

        MetricsRef fileMetric2;
        fileMetric2.Init(std::move(labels));
        MetricPtr fileCounter2 = fileMetric2.Get()->CreateCounter("filed2");
        fileCounter2->SetValue((uint64_t)222);
    }
    
    {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("project","project1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("logstore","logstore1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-hangzhou"));
        MetricsRef fileMetric3;
        fileMetric3.Init(labels);
        MetricPtr fileCounter3 = fileMetric3.Get()->CreateCounter("filed3");
        fileCounter3->SetValue((uint64_t)333);
    }

    ReadMetrics::GetInstance()->UpdateMetrics();
    
    Metrics* head = ReadMetrics::GetInstance()->GetHead();
    int count = 0;

    while(head) {
        head = head->GetNext();
        count ++;
    } 
    APSARA_TEST_EQUAL(count, 1);  

    MetricExportor::GetInstance()->PushMetrics(true);
}

void PushMetrics() {
    for (int i = 0; i < 10; i ++) {
        LOG_INFO(sLogger, ("PushMetricsCount", i));
        MetricExportor::GetInstance()->PushMetrics(true);
    }
}

void createMetrics(int count) {
    for (int i = 0; i < count; i ++) {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("num", std::to_string(i)));
        labels.emplace_back(std::make_pair<std::string, std::string>("count", std::to_string(count)));
        labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-beijing"));
        MetricsRef fileMetric;
        fileMetric.Init(std::move(labels));
        MetricPtr fileCounter = fileMetric.Get()->CreateCounter("filed1");
        fileCounter->SetValue((uint64_t)111);
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



void createRunningMetrics() {
    std::vector<std::pair<std::string, std::string>> labels;
    labels.emplace_back(std::make_pair<std::string, std::string>("region","cn-beijing"));
    MetricsRef fileMetric;
        

    fileMetric.Init(std::move(labels));
    MetricPtr fileCounter = fileMetric.Get()->CreateCounter("filed1");

    LOG_INFO(sLogger, ("createRunningMetrics", fileMetric.Get()));

    int count = 0;
    while(running) {
        fileCounter->SetValue((uint64_t)1);
        count++;
        sleep(1);
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

    std::thread tRunning1(createRunningMetrics);
    std::thread tRunning2(createRunningMetrics);

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
        APSARA_TEST_EQUAL(count, 2);  
        LOG_INFO(sLogger, ("FinalWriteCount", count));

    }

    {
        Metrics* head = ReadMetrics::GetInstance()->GetHead();
        int count = 0;
        while(head) {
            head = head->GetNext();
            count ++;
        }    
        APSARA_TEST_EQUAL(count, 2);  

        head = ReadMetrics::GetInstance()->GetHead();

        APSARA_TEST_EQUAL(head->GetLabels().size(), 1);  

        LOG_INFO(sLogger, ("FinalReadCount", count));
    }

    std::thread tRunning3(createRunningMetrics);
    std::thread tRunning4(createRunningMetrics);

    usleep(100);

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
        APSARA_TEST_EQUAL(count, 4);  
        LOG_INFO(sLogger, ("FinalWriteCount", count));

    }

    {
        Metrics* head = ReadMetrics::GetInstance()->GetHead();
        int count = 0;
        while(head) {
            head = head->GetNext();
            count ++;
        }    
        APSARA_TEST_EQUAL(count, 4);  
        LOG_INFO(sLogger, ("FinalReadCount", count));
    }

    
    MetricExportor::GetInstance()->PushMetrics(true);
    running.store(false);
    tRunning1.join();
    tRunning2.join();
    tRunning3.join();
    tRunning4.join();

    LOG_INFO(sLogger, ("end", "test"));
}


}// namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}