
/**
*@author dezhi.wangdezhi@alibaba-inc.com
*@date 12/29/17
*Copyright (c) 2016 Alibaba Group Holding Limited. All rights reserved.
*
*/
#ifndef ARGUSAGENT_HOSTHARDWAREINFO_H
#define ARGUSAGENT_HOSTHARDWAREINFO_H

#include <string>

namespace common {
    class HardwareInfo {
    public:
        std::string hostName;
        std::string mainIp;
        std::string ipList;
        std::string serviceTag;
        std::string clientOs;
        std::string clientOsVersion;
        std::string clientOsBit;
        std::string clientOsMainVersion;
        std::string appGroup;
        std::string appName;
        std::string idcName;
        std::string deviceModel;
        std::string serviceState;
        std::string securityDomain;
        bool isVm;
        int64_t timestamp;

        void display() const;
        std::string toString() const;
    };

}

#endif //ARGUSAGENT_HOSTHARDWAREINFO_H
