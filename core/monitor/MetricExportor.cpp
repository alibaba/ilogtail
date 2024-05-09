// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "MetricExportor.h"
#include "sender/Sender.h"
#include "log_pb/sls_logs.pb.h"
#include "LogtailMetric.h"
#include "config_manager/ConfigManager.h"
#include "LogFileProfiler.h"
#include "MetricConstants.h"
#include "go_pipeline/LogtailPlugin.h"
#include "app_config/AppConfig.h"
#include "common/TimeUtil.h"
#include "pipeline/PipelineManager.h"


using namespace sls_logs;
using namespace std;

namespace logtail {

MetricExportor::MetricExportor() : mSendInterval(60), mLastSendTime(time(NULL) - (rand() % (mSendInterval / 10)) * 10) {}

void MetricExportor::PushGoPluginMetrics() {
    std::vector<std::map<std::string, std::string>> goPluginMetircsList;
    LogtailPlugin::GetInstance()->GetPipelineMetrics(goPluginMetircsList);
    std::map<std::string, sls_logs::LogGroup*> goLogGroupMap;

    for (auto& item :  goPluginMetircsList) {
        std::string configName = "";
        std::string region = METRIC_REGION_DEFAULT;
        {
            // get the config_name label
            for (const auto& pair : item) {
                if (pair.first == "label.config_name") {
                    configName = pair.second;
                    break;
                }
            }
            if (!configName.empty()) {
                // get region info by config_name
                shared_ptr<Pipeline> p = PipelineManager::GetInstance()->FindPipelineByName(configName);
                if (p) {
                    FlusherSLS* pConfig = NULL;
                    pConfig = const_cast<FlusherSLS*>(static_cast<const FlusherSLS*>(p->GetFlushers()[0]->GetPlugin()));
                    if (pConfig) {
                        region = pConfig->mRegion;
                    }
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
            ProfileSender::GetInstance()->SendToProfileProject(ProfileSender::GetInstance()->GetDefaultProfileRegion(), *logGroup);
        } else {
            ProfileSender::GetInstance()->SendToProfileProject(iter->first, *logGroup);
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