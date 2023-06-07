#include "Metric.h"

namespace logtail {

void baseMetricAdd(BaseMetric *metric, uint64_t timestamp, uint64_t val) {
    metric->val += val;
    metric->timestamp = time(NULL);
};
}