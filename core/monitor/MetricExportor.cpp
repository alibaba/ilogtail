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

#include <filesystem>

#include "MetricConstants.h"
#include "MetricManager.h"
#include "app_config/AppConfig.h"
#include "common/FileSystemUtil.h"
#include "common/RuntimeUtil.h"
#include "common/TimeUtil.h"
#include "go_pipeline/LogtailPlugin.h"
#include "pipeline/PipelineManager.h"
#include "protobuf/sls/sls_logs.pb.h"

using namespace sls_logs;
using namespace std;

DECLARE_FLAG_STRING(metrics_report_method);

namespace logtail {

MetricExportor::MetricExportor() {
}

void MetricExportor::PushMetrics() {
    if ("sls" == STRING_FLAG(metrics_report_method)) {
        std::map<std::string, sls_logs::LogGroup*> logGroupMap;
        ReadMetrics::GetInstance()->ReadAsLogGroup(logGroupMap);
        SendToSLS(logGroupMap);
    } else if ("file" == STRING_FLAG(metrics_report_method)) {
        std::string metricsContent;
        ReadMetrics::GetInstance()->ReadAsFileBuffer(metricsContent);
        SendToLocalFile(metricsContent, "self-metrics");
    }
}

void MetricExportor::SendToSLS(std::map<std::string, sls_logs::LogGroup*>& logGroupMap) {
    std::map<std::string, sls_logs::LogGroup*>::iterator iter;
    for (iter = logGroupMap.begin(); iter != logGroupMap.end(); iter++) {
        sls_logs::LogGroup* logGroup = iter->second;
        GetProfileSender()->SendToProfileProject(iter->first, *logGroup);
        delete logGroup;
    }
}

void MetricExportor::SendToLocalFile(std::string& metricsContent, const std::string metricsFileNamePrefix) {
    const std::string metricsDirName = "self_metrics";
    const size_t maxFiles = 60; // 每分钟记录一次，最多保留1h的记录

    if (!metricsContent.empty()) {
        // 创建输出目录（如果不存在）
        std::string outputDirectory = GetAgentLogDir() + metricsDirName;
        Mkdirs(outputDirectory);

        std::vector<std::filesystem::path> metricFiles;

        for (const auto& entry : std::filesystem::directory_iterator(outputDirectory)) {
            if (entry.is_regular_file() && entry.path().filename().string().find(metricsFileNamePrefix) == 0) {
                metricFiles.push_back(entry.path());
            }
        }

        // 删除多余的文件
        if (metricFiles.size() > maxFiles) {
            std::sort(metricFiles.begin(),
                      metricFiles.end(),
                      [](const std::filesystem::path& a, const std::filesystem::path& b) {
                          return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                      });

            for (size_t i = maxFiles; i < metricFiles.size(); ++i) {
                std::filesystem::remove(metricFiles[i]);
            }
        }

        // 生成文件名
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time);
        std::ostringstream oss;
        oss << metricsFileNamePrefix << std::put_time(&now_tm, "-%Y-%m-%d_%H-%M-%S") << ".json";
        std::string filePath = PathJoin(outputDirectory, oss.str());

        // 写入文件
        std::ofstream outFile(filePath);
        if (!outFile) {
            LOG_ERROR(sLogger, ("Open file fail when print metrics", filePath.c_str()));
        } else {
            outFile << metricsContent;
            outFile.close();
        }
    }
}

} // namespace logtail