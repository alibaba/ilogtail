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
    // PluginMetric* pluginMetric = new PluginMetric();

    std::vector<std::pair<std::string, std::string>> labels;
    labels.emplace_back(std::make_pair<std::string, std::string>("project", "project1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("logstore", "logstore1"));
    labels.emplace_back(std::make_pair<std::string, std::string>("region", "cn-hangzhou"));

    MetricsRecordRef fileMetric;
    fileMetric.Init(labels);
    APSARA_TEST_EQUAL(fileMetric->GetLabels()->size(), 3);


    MetricNameValuePtr fileCounter = fileMetric->CreateCounter("filed1");
    fileCounter->SetValue(111UL);
    APSARA_TEST_EQUAL(fileCounter->GetValue(), 111);


    {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("project", "project1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("logstore", "logstore1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("region", "cn-hangzhou"));

        MetricsRecordRef fileMetric2;
        fileMetric2.Init(labels);
        MetricNameValuePtr fileCounter2 = fileMetric2->CreateCounter("filed2");
        fileCounter2->SetValue(222UL);
    }

    {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("project", "project1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("logstore", "logstore1"));
        labels.emplace_back(std::make_pair<std::string, std::string>("region", "cn-hangzhou"));
        MetricsRecordRef fileMetric3;
        fileMetric3.Init(labels);
        MetricNameValuePtr fileCounter3 = fileMetric3->CreateCounter("filed3");
        fileCounter3->SetValue(333UL);
    }

    ReadMetrics::GetInstance()->UpdateMetrics();

    MetricsRecord* head = ReadMetrics::GetInstance()->GetHead();
    int count = 0;

    while (head) {
        head = head->GetNext();
        count++;
    }
    APSARA_TEST_EQUAL(count, 1);

    MetricExportor::GetInstance()->PushMetrics(true);
}

void PushMetrics() {
    for (int i = 0; i < 10; i++) {
        LOG_INFO(sLogger, ("PushMetricsCount", i));
        MetricExportor::GetInstance()->PushMetrics(true);
    }
}

void createMetrics(int count) {
    for (int i = 0; i < count; i++) {
        std::vector<std::pair<std::string, std::string>> labels;
        labels.emplace_back(std::make_pair<std::string, std::string>("num", std::to_string(i)));
        labels.emplace_back(std::make_pair<std::string, std::string>("count", std::to_string(count)));
        labels.emplace_back(std::make_pair<std::string, std::string>("region", "cn-beijing"));
        MetricsRecordRef fileMetric;
        fileMetric.Init(labels);
        MetricNameValuePtr fileCounter = fileMetric->CreateCounter("filed1");
        fileCounter->SetValue(111UL);
    }
}

void UpdateMetrics() {
    for (int i = 0; i < 10; i++) {
        ReadMetrics::GetInstance()->UpdateMetrics();
        {
            MetricsRecord* head = WriteMetrics::GetInstance()->GetHead();
            int count = 0;
            while (head) {
                if (!head->IsDeleted()) {
                    count++;
                }
                head = head->GetNext();
            }
            LOG_INFO(sLogger, ("WriteCount", count));
        }
        {
            MetricsRecord* head = ReadMetrics::GetInstance()->GetHead();
            int count = 0;
            while (head) {
                head = head->GetNext();
                count++;
            }
            LOG_INFO(sLogger, ("ReadCount", count));
        }
    }
}


void createRunningMetrics() {
    std::vector<std::pair<std::string, std::string>> labels;
    labels.emplace_back(std::make_pair<std::string, std::string>("region", "cn-beijing"));
    MetricsRecordRef fileMetric;


    fileMetric.Init(labels);
    MetricNameValuePtr fileCounter = fileMetric->CreateCounter("filed1");
    int count = 0;
    while (running) {
        fileCounter->SetValue(111);
        usleep(100000);
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

    MetricsRecord* head = WriteMetrics::GetInstance()->GetHead();
    MetricsRecord* tmp = head;
    int count = 0;
    while (tmp != nullptr) {
        for (auto item = tmp->GetLabels()->begin(); item != tmp->GetLabels()->end(); ++item) {
            std::pair<std::string, std::string> pair = *item;
            LOG_INFO(sLogger, ("key", pair.first)("value", pair.second));
        }
        tmp = tmp->GetNext();
        count++;
    }
    // first test left 1, 1 + (1 + 2 + 3 + 4)
    APSARA_TEST_EQUAL(count, 11);
    LOG_INFO(sLogger, ("Count", count));

    // UpdateMetrics multi time
    for (int i = 0; i < 10; i++) {
        ReadMetrics::GetInstance()->UpdateMetrics();
    }

    {
        MetricsRecord* head = WriteMetrics::GetInstance()->GetHead();
        MetricsRecord* tmp = head;
        int count = 0;
        while (tmp != nullptr) {
            for (auto item = tmp->GetLabels()->begin(); item != tmp->GetLabels()->end(); ++item) {
                std::pair<std::string, std::string> pair = *item;
                LOG_INFO(sLogger, ("key", pair.first)("value", pair.second));
            }
            tmp = tmp->GetNext();
            count++;
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
        MetricsRecord* head = WriteMetrics::GetInstance()->GetHead();
        int count = 0;
        while (head) {
            if (!head->IsDeleted()) {
                count++;
            }
            head = head->GetNext();
        }
        APSARA_TEST_EQUAL(count, 2);
        LOG_INFO(sLogger, ("FinalWriteCount", count));
    }

    {
        MetricsRecord* head = ReadMetrics::GetInstance()->GetHead();
        int count = 0;
        while (head) {
            head = head->GetNext();
            count++;
        }
        APSARA_TEST_EQUAL(count, 2);

        head = ReadMetrics::GetInstance()->GetHead();

        APSARA_TEST_EQUAL(head->GetLabels()->size(), 1);

        LOG_INFO(sLogger, ("FinalReadCount", count));
    }

    std::thread tRunning3(createRunningMetrics);
    std::thread tRunning4(createRunningMetrics);

    usleep(100);

    // final UpdateMetrics after createMetrics
    ReadMetrics::GetInstance()->UpdateMetrics();
    {
        MetricsRecord* head = WriteMetrics::GetInstance()->GetHead();
        int count = 0;
        while (head) {
            if (!head->IsDeleted()) {
                count++;
            }
            head = head->GetNext();
        }
        APSARA_TEST_EQUAL(count, 4);
        LOG_INFO(sLogger, ("FinalWriteCount", count));
    }

    {
        MetricsRecord* head = ReadMetrics::GetInstance()->GetHead();
        int count = 0;
        while (head) {
            head = head->GetNext();
            count++;
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

    MetricsRecord* head = ReadMetrics::GetInstance()->GetHead();
    std::vector<MetricNameValuePtr> nameValues = head->GetMetricNameValues();

    APSARA_TEST_EQUAL(nameValues.size(), 1);
    LOG_INFO(sLogger, ("end", "test"));
}


} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}