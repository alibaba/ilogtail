#include <gtest/gtest.h>
#include "core/ExporterScheduler.h"

#include "common/FileUtils.h"
#include "common/HttpClient.h"
#include "common/ScopeGuard.h"
#include "common/UnitTestEnv.h"

#include "core/task_monitor.h"
#include "core/argus_manager.h"

using namespace common;
using namespace argus;
using namespace std;

class CoreExporterSchedulerTest : public testing::Test {
};

struct ExporterSchedulerStub : public ExporterScheduler {
    SyncQueue<int, Nonblock> chanSchedule{5};
    int scheduleCounter = 0;

    ~ExporterSchedulerStub() override {
        close();  // 一定要调用，否则等到ExporterScheduler中再调用时，chanScheduler就成了悬挂变量了
    }

    int scheduleOnce(ExporterSchedulerState &s) override {
        int code = ExporterScheduler::scheduleOnce(s);
        chanSchedule << (++scheduleCounter);
        return code;
    }
};

TEST_F(CoreExporterSchedulerTest, createScheduleItem00) {
    ExporterSchedulerStub scheduler;
    {
        ExporterItem item;
        item.type = 0;
        auto scheduleItem = scheduler.createScheduleItem(item);
        EXPECT_NE(nullptr, dynamic_cast<ExporterMetric *>(scheduleItem->pExporterMetric.get()));
        EXPECT_EQ(nullptr, scheduleItem->pExporterMetricStatus.get());
    }
    {
        ExporterItem item;
        item.type = 0;
        item.addStatus = 1;
        auto scheduleItem = scheduler.createScheduleItem(item);
        EXPECT_NE(nullptr, dynamic_cast<ExporterMetric *>(scheduleItem->pExporterMetric.get()));
        EXPECT_NE(nullptr, scheduleItem->pExporterMetricStatus.get());
    }
}

TEST_F(CoreExporterSchedulerTest, createScheduleItem01) {
    ExporterSchedulerStub scheduler;
    {
        ExporterItem item;
        item.type = 1;
        auto scheduleItem = scheduler.createScheduleItem(item);
        EXPECT_NE(nullptr, dynamic_cast<AliMetric *>(scheduleItem->pExporterMetric.get()));
        EXPECT_EQ(nullptr, scheduleItem->pExporterMetricStatus.get());
    }
    {
        ExporterItem item;
        item.type = 1;
        item.addStatus = 1;
        item.target = "https://www.baidu.com:4300";
        auto scheduleItem = scheduler.createScheduleItem(item);
        EXPECT_NE(nullptr, dynamic_cast<AliMetric *>(scheduleItem->pExporterMetric.get()));
        EXPECT_NE(nullptr, scheduleItem->pExporterMetricStatus.get());
    }
}

TEST_F(CoreExporterSchedulerTest, Test) {
    auto pScheduler = std::make_shared<ExporterSchedulerStub>();
    pScheduler->factor = 5_ms;
    pScheduler->runIt();

    TaskManager tm;
    auto *oldTm = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(oldTm));

    ArgusManager am;
    am.swap(pScheduler);
    auto *oldAm = SingletonArgusManager::swap(&am);
    defer(SingletonArgusManager::swap(oldAm));

    auto pTask = std::make_shared<TaskMonitor>();
    string taskJson;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/task1.json").string(), taskJson);
    EXPECT_GT(pTask->ParseExporterTaskInfo(taskJson), 0);

    int tmp;
    pScheduler->chanSchedule.Take(tmp);

    EXPECT_EQ(pScheduler->m_state.size(), size_t(1));
    for (auto const &it: pScheduler->m_state) {
        cout << "key: " << it.first << endl;
    }

    taskJson.clear();
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/task2.json").string(), taskJson);
    pTask->ParseExporterTaskInfo(taskJson);

    // 两个任务
    pScheduler->chanSchedule.Take(tmp);
    pScheduler->chanSchedule.Take(tmp);

    for (auto const &it: pScheduler->m_state) {
        cout << "key:" << it.first << endl;
    }
    EXPECT_EQ(pScheduler->m_state.size(), size_t(2));

    taskJson.clear();
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/task3.json").string(), taskJson);
    pTask->ParseExporterTaskInfo(taskJson);

    CommonMetric m;
    pScheduler->GetStatus(m);
    LogInfo("{}", m.toString());
    EXPECT_EQ(m.name, "exporter_status");
}

TEST_F(CoreExporterSchedulerTest, BuildAddStatus) {
    string result = ExporterScheduler::BuildAddStatus("testName", 100, "timeout", 0);
    const char *expect = R"XX(
__argus_exporter_status__{__argus_exporter_name__="testName",__argus_exporter_code__="100",__argus_exporter_error_msg__="timeout"} 0)XX";
    EXPECT_EQ(result, expect + 1);
}

TEST_F(CoreExporterSchedulerTest, AliMetricCollector_Collect) {
    AliMetricCollector collector(ExporterItem{}, 1_s);
    EXPECT_EQ(collector.activeIndex, size_t(0));
    auto httpCurl = [](const common::HttpRequest &, common::HttpResponse &resp) -> int {
        resp.resCode = 403;
        return (int) HttpClient::Success;
    };

    int code = 0;
    std::string errMsg, result;
    EXPECT_FALSE(collector.collect(httpCurl, code, errMsg, result));
    EXPECT_EQ(collector.activeIndex, size_t(1));
}

TEST_F(CoreExporterSchedulerTest, schedulerOnce) {
    struct MetricStub : BaseParseMetric {
        MetricStub() : BaseParseMetric(std::vector<MetricFilterInfo>{}, std::vector<LabelAddInfo>{}) {
        }

        int GetCommonMetrics(std::vector<common::CommonMetric> &commonMetrics) override {
            commonMetrics.resize(commonMetrics.size() + 1);
            return 1;
        }


        int addMetricCount = 0;
        ParseErrorType AddMetric(const std::string &metricStr) override {
            addMetricCount++;
            return ParseErrorType::PARSE_SUCCESS;
        }
    };

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    ExporterSchedulerStub scheduler;
    scheduler.httpCurl = [](const common::HttpRequest &, common::HttpResponse &resp) {
        resp.resCode = 200;
        return 0;
    };

    ExporterItem item;
    item.type = 1;
    item.addStatus = 1;
    auto scheduleItem = scheduler.createScheduleItem(item);
    scheduleItem->pExporterMetric = std::make_shared<MetricStub>();
    scheduleItem->pExporterMetricStatus = std::make_shared<MetricStub>();

    EXPECT_EQ(0, scheduler.scheduleOnce(*scheduleItem));

    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;

    EXPECT_EQ(1, std::dynamic_pointer_cast<MetricStub>(scheduleItem->pExporterMetric)->addMetricCount);
    EXPECT_EQ(1, std::dynamic_pointer_cast<MetricStub>(scheduleItem->pExporterMetricStatus)->addMetricCount);
    EXPECT_NE(std::string::npos, cache.find("will add exporter status for"));
    EXPECT_NE(std::string::npos, cache.find("send metrics 2 to outputChannel"));
}
