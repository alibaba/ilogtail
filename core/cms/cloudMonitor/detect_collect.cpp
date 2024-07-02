#ifdef WIN32
#	include <Windows.h>
#endif
#include "detect_collect.h"

#include <memory>
#include "detect/detect_schedule_status.h"
#include "common/Logger.h"
#include "detect/detect_schedule.h"
#include "common/Config.h"

using namespace std;
using namespace common;

namespace cloudMonitor {
    DetectCollect::DetectCollect(): mModuleName("detect") {
        LogInfo("load detect");
    }

    int DetectCollect::Init() {
        if (!SingletonConfig::Instance()->GetValue("agent.enable.detect.module", true)) {
            LogInfo("detect module is disabled");
            return -1;
        }
#ifdef ENABLE_CLOUD_MONITOR
        const int defaultMinThread = 1;
#else
        const int defaultMinThread = 10;
#endif
        int detectThread = SingletonConfig::Instance()->GetValue<int>("cms.detect.thread", defaultMinThread);
        int maxDetectThread = SingletonConfig::Instance()->GetValue<int>("cms.detect.max.thread", 20);
        if (detectThread > maxDetectThread || detectThread <= 0) {
            detectThread = defaultMinThread;
            maxDetectThread = 20;
        }
        mPDetectSchedule = std::make_shared<DetectSchedule>(false);
        SingletonDetectScheduleStatus::Instance()->SetDetectSchedule(mPDetectSchedule);
        mPDetectSchedule->Start(detectThread, maxDetectThread);

        return 0;
    }

    DetectCollect::~DetectCollect() {
        LogInfo("unload detect");
    }

    int DetectCollect::Collect(string &data) const {
        if (mPDetectSchedule == nullptr) {
            LogWarn("mPDetectSchedule should init");
            return -1;
        }
        CollectData collectData;
        mPDetectSchedule->Collect(collectData);
        if (collectData.dataVector.empty()) {
            return 0;
        }
        collectData.moduleName = mModuleName;
        ModuleData::convertCollectDataToString(collectData, data);
        return static_cast<int>(data.size());
    }
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DLL
#include "cloud_module_macros.h"

using cloudMonitor::DetectCollect;

IMPLEMENT_CMS_MODULE(detect) {
    return module::NewHandler<DetectCollect>();
}
