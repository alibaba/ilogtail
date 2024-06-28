#include "cloud_monitor_const.h"
#include "common/ChronoLiteral.h"

namespace cloudMonitor {
    //默认采集周期为15秒
    const std::chrono::seconds kDefaultInterval = 15_s;
}