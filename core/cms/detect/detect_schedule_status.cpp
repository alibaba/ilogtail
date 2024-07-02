//
// Created by 韩呈杰 on 2024/3/13.
//
#include "detect_schedule_status.h"
#include "detect_schedule.h"

void DetectScheduleStatus::GetStatus(common::CommonMetric &metric) {
    auto tmp = mDetectSchedule.lock();
    if (tmp != nullptr) {
        tmp->GetStatus(metric);
    }
}
