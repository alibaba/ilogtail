#ifndef ARGUS_DETECT_SCHEDULE_STATUS_H
#define ARGUS_DETECT_SCHEDULE_STATUS_H

#include <memory>
#include "common/Singleton.h"

namespace common {
    struct CommonMetric;
    class DetectSchedule;
}

class DetectScheduleStatus {
public:
    void SetDetectSchedule(std::weak_ptr<common::DetectSchedule> pDetectSchedule) {
        mDetectSchedule = std::move(pDetectSchedule);
    }

    void GetStatus(common::CommonMetric &metric);

private:
    std::weak_ptr<common::DetectSchedule> mDetectSchedule;
};

typedef common::Singleton<DetectScheduleStatus> SingletonDetectScheduleStatus;

#endif
