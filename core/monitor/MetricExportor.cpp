#include "MetricExportor.h"
#include "sender/Sender.h"
#include "log_pb/sls_logs.pb.h"
#include "common/LogtailCommonFlags.h"

using namespace sls_logs;
using namespace std;

namespace logtail {

MetricExportor::MetricExportor() {
    mSendInterval = 10;
    mSnapshotInterval = 10;
    mLastSendTime = 0;
    mLastSnapshotTime = 0;
}

void MetricExportor::PushMetrics() {
    ReadMetrics::GetInstance()->UpdateMetrics();
    std::map<std::string, sls_logs::LogGroup> logGroupMap;
    ReadMetrics::GetInstance()->ReadAsLogGroup(logGroupMap);
    // TODO: send
    std::map<std::string, sls_logs::LogGroup>::iterator iter;
    for (iter = logGroupMap.begin(); iter != logGroupMap.end(); iter ++) {
        LOG_INFO(sLogger, ("Region", iter->first));
        sls_logs::LogGroup logGroup = iter->second;
        logGroup.set_category("shennong_log_profile");
        for (int i = 0; i < logGroup.logs_size(); ++i) {
            const sls_logs::Log& log = logGroup.logs(i);
            for (int j = 0; j < log.contents_size(); ++j) {
                const sls_logs::Log_Content& content = log.contents(j);
                LOG_INFO(sLogger, ("contentKey", content.key())("contentValue", content.value()));
            }
        }
    }
}
}