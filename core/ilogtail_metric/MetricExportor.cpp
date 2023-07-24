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
    LOG_INFO(sLogger, ("pushMetrics", ""));
    ReadMetrics::GetInstance()->UpdateMetrics();
    sls_logs::LogGroup logGroup;
    ReadMetrics::GetInstance()->ReadAsLogGroup(logGroup);
    for (int i = 0; i < logGroup.logs_size(); ++i) {
        const sls_logs::Log& log = logGroup.logs(i);
        for (int j = 0; j < log.contents_size(); ++j) {
            const sls_logs::Log_Content& content = log.contents(j);
            LOG_INFO(sLogger, ("contentKey", content.key())("contentValue", content.value()));
        }
    }
}

}