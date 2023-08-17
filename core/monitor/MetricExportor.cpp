#include "MetricExportor.h"
#include "sender/Sender.h"
#include "log_pb/sls_logs.pb.h"
#include "LogtailMetric.h"
#include "log_pb/sls_logs.pb.h"

using namespace sls_logs;
using namespace std;

namespace logtail {

MetricExportor::MetricExportor() {}

void MetricExportor::PushMetrics() {
    ReadMetrics::GetInstance()->UpdateMetrics();
    std::map<std::string, sls_logs::LogGroup> logGroupMap;
    ReadMetrics::GetInstance()->ReadAsLogGroup(logGroupMap);
    // TODO: send
}
}