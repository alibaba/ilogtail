
/**
*@author dezhi.wangdezhi@alibaba-inc.com
*@date 1/1/18
*Copyright (c) 2016 Alibaba Group Holding Limited. All rights reserved.
*
*/
#ifndef ARGUSAGENT_HARDWAREINFOPROVIDER_H
#define ARGUSAGENT_HARDWAREINFOPROVIDER_H

#include "HardwareInfo.h"
#include "InstanceLock.h"
#include "Singleton.h"

namespace common {
#define HARDWAREINFO_INTREVAL 60

#include "common/test_support"
class HardwareInfoProvider {
public:
    HardwareInfoProvider();
    void getHostHardwareInfo(HardwareInfo &hardwareInfo);
private:

    bool updateHardwareInfo();
    bool parseJson(const std::string &);

    HardwareInfo hardwareInfo;
    mutable InstanceLock hardwareInfoLock;
};
#include "common/test_support"

    typedef Singleton<HardwareInfoProvider> SingletonHardwareInfoProvider;
}


#endif //ARGUSAGENT_HARDWAREINFOPROVIDER_H
