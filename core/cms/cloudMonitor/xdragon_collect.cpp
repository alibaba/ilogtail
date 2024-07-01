#include "xdragon_collect.h"

#include "common/Logger.h"
#include "common/PropertiesFile.h"
#include "common/Common.h"
#include "common/ModuleData.h"
#include "common/TimeProfile.h"

using namespace std;
using namespace common;

// See: https://aone.alibaba-inc.com/req/21699740
// 神龙(Xdragon)相关的监控: 按指定间隔运行程序，并将程序输出文本上报。主要分为两部分：
// 1. 配置，定义了需运行的程序路径及运行间隔
// 2. 监控，运行程序并随普通指标上报

namespace cloudMonitor {
    XDragonCollect::XDragonCollect() {
        mModuleName = "xdragon";
        mTotalCount = 30 * 60;
#ifdef WIN32
        mBinFile=R"(C:\ProgramData\aliyun\xdragon_hwqc\cloudhwm.exe)";
        mIniFile=R"(C:\ProgramData\aliyun\xdragon_hwqc\config.ini)";
#else
        mBinFile = "/usr/local/xdragon_hwqc/cloudhwm";
        mIniFile = "/usr/local/xdragon_hwqc/config.ini";
#endif
        LogInfo("load xdragon");
    }

    int XDragonCollect::Init() {
        mCount = 0;
        return CheckEvn()? 0: -1;
    }

    XDragonCollect::~XDragonCollect() {
        LogInfo("unload xdragon");
    }

    int XDragonCollect::Collect(string &data) {
        mCount++;
        if (mCount % mTotalCount == 0) {
            TimeProfile timeProfile;
            string result;
            if (GetCollectResult(result)) {
                CollectData collectData;
                collectData.moduleName = mModuleName;
                MetricData metricData;
                metricData.tagMap["metricName"] = "xdragon";
                metricData.valueMap["metricValue"] = 0;
                metricData.tagMap["ns"] = "acs/xdragon";
                metricData.tagMap["hwqc"] = result;
                collectData.dataVector.push_back(metricData);
                ModuleData::convertCollectDataToString(collectData, data);

                LogDebug("collect xdragon spend {} ms", timeProfile.millis());
                return static_cast<int>(data.size());
            }
        }

        return 0;
    }

    bool XDragonCollect::CheckEvn() {
        unique_ptr<PropertiesFile> pPro(new PropertiesFile(mIniFile));
        mBinFile = pPro->GetValue("binary", mBinFile);
        if (!fs::exists(mBinFile)) {
            LogInfo("xdragon bin file({}) is not exist, skip!!!", mBinFile.c_str());
            return false;
        }

        pPro->GetValue("interval", mTotalCount, 1800);
        const bool valid = mTotalCount > 0;
        Log((valid ? LEVEL_INFO : LEVEL_WARN), "collect interval({}) is valid: {}", mTotalCount, valid);

        return valid;
    }

    bool XDragonCollect::GetCollectResult(string &result) const {
        size_t index;
#ifdef WIN32
        index= mBinFile.find_last_of('\\');
#else
        index = mBinFile.find_last_of('/');
#endif
        if (index == string::npos) {
            return false;
        }
        string cwd = mBinFile.substr(0, index + 1);
#ifdef WIN32
        cwd ="cd /d "+cwd;
#else
        cwd = "cd " + cwd;
#endif
        string cmdline = cwd + " && " + mBinFile;
        // char buf[2049];
        // memset(buf, 0, 2049);
        std::string buf;
        if (getCmdOutput(cmdline, buf) == 0) {
            result = buf;
            return true;
        } else {
            LogWarn("exec {} error", cmdline.c_str());
            return false;
        }
    }
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(xdragon) {
    return module::NewHandler<cloudMonitor::XDragonCollect>();
}
