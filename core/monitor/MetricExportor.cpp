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

const string METRIC_REGION_DEFAULT = "default";
const string METRIC_SLS_LOGSTORE_NAME = "shennong_log_profile";

const std::string METRIC_EXPORT_TYPE_GO = "direct";
const std::string METRIC_EXPORT_TYPE_CPP = "cpp_provided";

MetricExportor::MetricExportor() : mSendInterval(60), mLastSendTime(time(NULL) - (rand() % (mSendInterval / 10)) * 10) {
}

void MetricExportor::PushMetrics(bool forceSend) {
    int32_t curTime = time(NULL);
    if (!forceSend && (curTime - mLastSendTime < mSendInterval)) {
        return;
    }
}

} // namespace logtail