//
// Created by dezhi.wangdezhi on 12/29/17.
//
#include "common/HardwareInfo.h"
#include "common/Logger.h"

void common::HardwareInfo::display() const {
    LogInfo(toString());
}

std::string common::HardwareInfo::toString() const {
    return fmt::format(
            "isVm={},hostName={},appName={},appGroup={},idcName={},serviceState={},deviceModel={},sn={},ips={},securityDomain={}",
            isVm, hostName, appName, appGroup, idcName, serviceState, deviceModel, serviceTag, ipList, securityDomain
    );
}

// std::string common::HardwareInfo::getIpListString(const std::string &separator) const {
//     std::string tmp = ipList;
//
//     tmp = Trim(tmp, "|");
//     // if (*tmp.begin() == '|') {
//     //     tmp.erase(tmp.begin());
//     // }
//     // if (*tmp.rbegin() == '|') {
//     //     tmp = tmp.substr(0, tmp.length() - 1);
//     // }
//     return StringUtils::replaceAll(tmp, "|", ",");
// }
