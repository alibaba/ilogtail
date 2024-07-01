#ifndef _DETECT_COLLECT_H_
#define _DETECT_COLLECT_H_

#include <string>
#include <memory>

namespace common {
    class DetectSchedule;
}

namespace cloudMonitor {
#include "common/test_support"
    class DetectCollect {
    public:
        DetectCollect();

        int Init();

        ~DetectCollect();

        int Collect(std::string &data) const;

    private:
        const std::string mModuleName;
        std::shared_ptr<common::DetectSchedule> mPDetectSchedule;
    };
#include "common/test_support"
}
#endif 