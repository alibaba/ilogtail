#include "MetricExportor.h"
#include "sender/Sender.h"
#include "log_pb/sls_logs.pb.h"
#include "LogtailMetric.h"
#include "config_manager/ConfigManager.h"
#include "LogFileProfiler.h"
#include "MetricConstants.h"
#include "go_pipeline/LogtailPlugin.h"
#include "app_config/AppConfig.h"

using namespace sls_logs;
using namespace std;

namespace logtail {

MetricExportor::MetricExportor() : mSendInterval(60), mLastSendTime(time(NULL) - (rand() % (mSendInterval / 10)) * 10) {}

void MetricExportor::PushGoPluginMetrics() {
    std::vector<std::map<std::string, std::string>> goPluginMetircsList;
    LogtailPlugin::GetInstance()->GetPipelineMetrics(goPluginMetircsList);
    std::map<std::string, sls_logs::LogGroup*> goLogGroupMap;
    std::map<std::string, std::string> tmpConfigNameToRegion;
    for (auto& item :  goPluginMetircsList) {
        std::string configName = "";
        std::string region = METRIC_REGION_DEFAULT;
        {
            for (const auto& pair : item) {
                if (pair.first == "label.config_name") {
                    configName = pair.second;
                    break;
                }
            }
            if (!configName.empty()) {
                auto search = tmpConfigNameToRegion.find(configName);
                if (search != tmpConfigNameToRegion.end()) {
                    region = search->second;
                } else {
                    Config* config = ConfigManager::GetInstance()->FindConfigByName(configName);
                    tmpConfigNameToRegion.insert(std::make_pair(configName, config->mRegion));
                }
            } 
        }
        Log* logPtr = nullptr;
        auto LogGroupIter = goLogGroupMap.find(region);
        if (LogGroupIter != goLogGroupMap.end()) {
            sls_logs::LogGroup* logGroup = LogGroupIter->second;
            logPtr = logGroup->add_logs();
        } else {
            sls_logs::LogGroup* logGroup = new sls_logs::LogGroup();
            logPtr = logGroup->add_logs();
            goLogGroupMap.insert(std::pair<std::string, sls_logs::LogGroup*>(region, logGroup));
        }
        auto now = GetCurrentLogtailTime();
        SetLogTime(logPtr,
                   AppConfig::GetInstance()->EnableLogTimeAutoAdjust() ? now.tv_sec + GetTimeDelta() : now.tv_sec);
        for (const auto& pair : item) {
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(pair.first);
            contentPtr->set_value(pair.second);
        }   
    }
    SendMetrics(goLogGroupMap);
}

void MetricExportor::SendMetrics(std::map<std::string, sls_logs::LogGroup*>& logGroupMap) {
    std::map<std::string, sls_logs::LogGroup*>::iterator iter;
    for (iter = logGroupMap.begin(); iter != logGroupMap.end(); iter ++) {
        sls_logs::LogGroup* logGroup = iter->second;
        logGroup->set_category(METRIC_SLS_LOGSTORE_NAME);
        logGroup->set_source(LogFileProfiler::mIpAddr);
        logGroup->set_topic(METRIC_TOPIC_TYPE);
        if (METRIC_REGION_DEFAULT == iter->first) {
            mProfileSender.SendToProfileProject(ConfigManager::GetInstance()->GetDefaultProfileRegion(), *logGroup);
        } else {
            mProfileSender.SendToProfileProject(iter->first, *logGroup);
        }
        delete logGroup;
    }
}

void MetricExportor::PushMetrics(bool forceSend) {
    int32_t curTime = time(NULL);
    if (!forceSend && (curTime - mLastSendTime < mSendInterval)) {
        return;
    }

    if (LogtailPlugin::GetInstance()->IsPluginOpened()) {
        PushGoPluginMetrics();
    }

    ReadMetrics::GetInstance()->UpdateMetrics();    
    std::map<std::string, sls_logs::LogGroup*> logGroupMap;
    ReadMetrics::GetInstance()->ReadAsLogGroup(logGroupMap);

    SendMetrics(logGroupMap);
}
}