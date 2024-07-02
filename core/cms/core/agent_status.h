#ifndef AGENT_CORE_STATUS_H
#define AGENT_CORE_STATUS_H

#include <string>
#include "common/Singleton.h"
#include "common/SafeMap.h"

namespace argus {
    enum AGENT_STATUS_TYPE {
        AGENT_RESOURCE_STATUS = 1,
        AGENT_COREDUMP_STATUS = 2,
        AGENT_START_STATUS = 3,
    };

    struct AgentMetricStatus {
        std::chrono::system_clock::time_point timestamp;
        int count = 0;
        int maxCount = 3;
        std::string metricName;
    };

#include "common/test_support"
class AgentStatus {
public:
    AgentStatus();
    ~AgentStatus();
    int UpdateAgentStatus(AGENT_STATUS_TYPE type);
    bool GetAgentStatus(AgentMetricStatus * = nullptr) const;
    int GetAgentStatus(AGENT_STATUS_TYPE type) const;

    static int maxCount(AGENT_STATUS_TYPE type);
private:
    int InitAgentStatus();
    int LoadAgentStatus();
    static int SaveAgentStatus(const std::string &file, const std::map<AGENT_STATUS_TYPE, AgentMetricStatus> &);
private:
    SafeMap<AGENT_STATUS_TYPE, AgentMetricStatus> mMetricMap;
    std::chrono::seconds mEffectInterval{0};
    std::string mAgentStatusFile;
    // int mMaxResouceCount = 0;
    // int mMaxCoredumpCount = 0;
    // int mMaxStartCount = 0;
};
#include "common/test_support"

    typedef common::Singleton<AgentStatus> SingletonAgentStatus;
}
#endif 
