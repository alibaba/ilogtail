//
// Created by dezhi.wangdezhi on 1/1/18.
//

#include "common/HardwareInfoProvider.h"
#include "common/JsonValueEx.h"
#include "common/Logger.h"
#include "common/SystemUtil.h"
#include "common/HttpClient.h"
#include "common/Chrono.h"

using namespace std;

namespace common {
    const string hostInfoPath1 = "/usr/sbin/hostinfo -a -f json 2>&1";
    // const string keyClientOs = "clientOs";
    // const string keyClientOsBit = "clientOsBit";
    // const string keyClientOsMainVersion = "clientOsMainVersion";
    // const string keyClientOsVersion = "clientOsVersion";
    // const string keyHostname = "hostName";
    // const string keyIpList = "ipList";
    // const string keyMainIp = "mainIp";
    // const string keyIp = "ip";
    // 优先使用这个获取sn,/usr/sbin/hostinfo优先支持sn
    // const string keySn = "sn";
    // const string keyServiceTag = "serviceTag";
    // const string keyAppName = "appName";
    // const string keyAppGroup = "appGroup";
    // 机房信息
    // const string keyIdcName = "idcName";
    // const string keyServiceState = "serviceState";
    // const string keyIsVm = "isVm";
    // const string keyDeviceModel = "deviceModel";

    // const string keyConfig = "Config";
    // const string keySN = "SN";
    // const string keyEnv = "Env";
    // const string keyAliContainerIp = "ali.container_ip";
    // const string keyDockerHostname = "Hostname";
    // const string keyLabels = "Labels";

    // const string keyCmdb = "cmdb";
    // const string keySecurityDomain = "securityDomain";
    // 新的local相关字段
    // const string keyLocal = "local";
    // const string keyLocalOs = "os";
    // const string keyLocalOsVersion = "osVersion";
    // const string keyLocalOsBit = "osBit";
    // const string keyLocalOsMainVersion = "osMainVersion";

    void HardwareInfoProvider::getHostHardwareInfo(HardwareInfo &hwInfo) {
        LockGuard lockGuard(hardwareInfoLock);
        int64_t currentTime = NowSeconds();
        if (currentTime >= this->hardwareInfo.timestamp + HARDWAREINFO_INTREVAL) {
            updateHardwareInfo();
        }
        hwInfo = this->hardwareInfo;
        //进行兜底补偿
        hwInfo.mainIp = (hwInfo.mainIp.empty() ? SystemUtil::getMainIp() : hwInfo.mainIp);
        hwInfo.hostName = (hwInfo.hostName.empty() ? SystemUtil::getHostname() : hwInfo.hostName);
        hwInfo.serviceTag = (hwInfo.serviceTag.empty() ? SystemUtil::getSn() : hwInfo.serviceTag);
    }

    bool HardwareInfoProvider::updateHardwareInfo() {
#ifdef WIN32
        return true;
#else
        // int maxContentLength = 2046;
        //hostInfoPath1为新的hostinfo,
        // char *content = new char[maxContentLength + 1]{'\0'};
        // ResourceGuard resourceGuard([content] { delete[] content; });
        std::string content;
        getCmdOutput(hostInfoPath1, content);

        return parseJson(content);
#endif
    }

    HardwareInfoProvider::HardwareInfoProvider() {
#ifndef WIN32
        updateHardwareInfo();
#endif
    }

    bool HardwareInfoProvider::parseJson(const std::string &strJson) {
        std::string error;
        json::Object value = json::parseObject(strJson, error);
        if (value.isNull()) {
            LogError("get hostinfo, parse cms [{}] output <{}> failed: {}", hostInfoPath1, strJson, error);
            return false;
        }

        HardwareInfo tmpInfo;
        json::Object local = value.getObject("local");
        if (!local.empty()) {
            tmpInfo.clientOs = local.getString("os");
            tmpInfo.clientOsVersion = local.getString("osVersion");
            tmpInfo.clientOsBit = local.getString("osBit");
            tmpInfo.clientOsMainVersion = local.getString("osMainVersion");
            tmpInfo.ipList = local.getString("ipList");
            tmpInfo.serviceTag = local.getString("sn");
            tmpInfo.hostName = local.getString("hostName");
            tmpInfo.mainIp = local.getString("ip");
        }

        json::Object cmdb = value.getObject("cmdb");
        if (!cmdb.empty()) {
            //新的hostinfo需要额外读取appGroup,appName,idcName,deviceModel,serviceState字段，从cmdb读取
            cmdb.get("appGroup", tmpInfo.appGroup);
            cmdb.get("appName", tmpInfo.appName);
            cmdb.get("idcName", tmpInfo.idcName);
            cmdb.get("deviceModel", tmpInfo.deviceModel);
            cmdb.get("serviceState", tmpInfo.serviceState);
            cmdb.get("securityDomain", tmpInfo.securityDomain);
            cmdb.get("isVm", tmpInfo.isVm); // bool
        }

        bool fail = local.empty() && cmdb.empty();
        if (!fail) {
            tmpInfo.timestamp = NowSeconds();
            tmpInfo.display();

            LockGuard lockGuard(hardwareInfoLock);
            hardwareInfo = tmpInfo;
        }
        return !fail;
    }
}