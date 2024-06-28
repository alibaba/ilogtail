#include "agent_status.h"

#include <vector>
#include <map>
#include <boost/core/demangle.hpp>

#include "common/Logger.h"
#include "common/FileUtils.h"
#include "common/StringUtils.h"
#include "common/Chrono.h"
#include "common/Config.h"

using namespace common;
using namespace std;

struct tagAgentStatusConfig {
    int maxCount = 3;
    argus::AGENT_STATUS_TYPE type = argus::AGENT_START_STATUS;
    const char *cfgKey = nullptr;

    tagAgentStatusConfig() = default;

    tagAgentStatusConfig(int count, argus::AGENT_STATUS_TYPE t, const char *k)
            : maxCount(count), type(t), cfgKey(k) {
    }
};

static std::once_flag agentStatusMapOnce;
static auto &agentStatusMap = *new std::map<std::string, tagAgentStatusConfig>{
        {"resourceCount", {4, argus::AGENT_RESOURCE_STATUS, "agent.status.resource.count"}},
        {"coredumpCount", {3, argus::AGENT_COREDUMP_STATUS, "agent.status.coredump.count"}},
        {"startCount",    {20, argus::AGENT_START_STATUS, "agent.status.start.count"}},
};

namespace argus {
    AgentStatus::AgentStatus() {
        Config *cfg = SingletonConfig::Instance();
        //默认周期为1天
        mEffectInterval = std::chrono::seconds{cfg->GetValue("agent.status.interval", 3600 * 24)};
        std::call_once(agentStatusMapOnce, [&]() {
            for (auto &it: agentStatusMap) {
                it.second.maxCount = cfg->GetValue(it.second.cfgKey, it.second.maxCount);
            }
            // //默认资源不能超过4次
            // // mMaxResouceCount = cfg->GetValue("agent.status.resource.count", 4);
            // mMaxCountMap[AGENT_RESOURCE_STATUS] = cfg->GetValue("agent.status.resource.count", 4);
            // //默认coredump不能超过3次
            // // mMaxCoredumpCount = cfg->GetValue("agent.status.coredump.count", 3);
            // mMaxCountMap[AGENT_COREDUMP_STATUS] = cfg->GetValue("agent.status.coredump.count", 3);
            // //默认不能被拉起20次
            // // mMaxStartCount = cfg->GetValue("agent.status.start.count", 20);
            // mMaxCountMap[AGENT_START_STATUS] = cfg->GetValue("agent.status.start.count", 20);;
        });
        mAgentStatusFile = (cfg->getBaseDir() / cfg->GetValue("agent.status.file", "agent.status")).string();
        InitAgentStatus();
        LoadAgentStatus();
    }

    AgentStatus::~AgentStatus() = default;

    int AgentStatus::InitAgentStatus() {
        for (const auto &it: agentStatusMap) {
            AgentMetricStatus item;
            item.metricName = it.first;
            item.maxCount = it.second.maxCount;
            mMetricMap.Set(it.second.type, item);
        }

        return 0;
    }

    // static
    int AgentStatus::maxCount(AGENT_STATUS_TYPE type) {
        for (const auto &it: agentStatusMap) {
            if (it.second.type == type) {
                return it.second.maxCount;
            }
        }
        return -1;
    }

    int AgentStatus::LoadAgentStatus() {
        vector<string> lines;
        if (fs::exists(mAgentStatusFile) && FileUtils::ReadFileLines(mAgentStatusFile, lines) <= 0) {
			LogWarn("read agent-status file({}) error", mAgentStatusFile);
            return -1;
        }
        // int64_t now = NowSeconds();
        auto now = std::chrono::Now<std::chrono::seconds>();
        for (std::string line: lines) {
            vector<string> elems = StringUtils::split(line, "|", true);
            if (elems.size() != 3) {
                LogWarn("invalid line({}) in agent-stats file", line);
                continue;
            }
            AgentMetricStatus agentMetricStats;
            agentMetricStats.timestamp = std::chrono::FromSeconds(convert<int64_t>(elems[0]));
            if (agentMetricStats.timestamp + mEffectInterval < now) {
                continue;
            }
            convert(elems[2], agentMetricStats.count);
            agentMetricStats.metricName = elems[1];

            auto mappedTypeIt = agentStatusMap.find(elems[1]);
            if (mappedTypeIt != agentStatusMap.end()) {
                agentMetricStats.maxCount = mappedTypeIt->second.maxCount;
                mMetricMap.Set(mappedTypeIt->second.type, agentMetricStats);
            } else {
                LogWarn("invalid metric({}) in agent-status file, {} .", elems[1], mAgentStatusFile);
                continue;
            }
            // if (elems[1] == RESOURCE_COUNT) {
            //     mMetricMap[AGENT_RESOURCE_STATUS] = agentMetricStats;
            // } else if (elems[1] == COREDUMP_COUNT) {
            //     mMetricMap[AGENT_COREDUMP_STATUS] = agentMetricStats;
            // } else if (elems[1] == START_COUNT) {
            //     mMetricMap[AGENT_START_STATUS] = agentMetricStats;
            // } else {
            //     LogWarn("invalid metric({}) in agent-stats file", elems[1].c_str());
            //     continue;
            // }
        }
        return 0;
    }

    int AgentStatus::SaveAgentStatus(const std::string &file, const std::map<AGENT_STATUS_TYPE, AgentMetricStatus> &d) {
        // string total;
        std::stringstream ss;
        for (const auto &it: d) {
            // string line = StringUtils::NumberToString(it.second.timestamp) + "|";
            // line += it.second.metricName + "|";
            // line += StringUtils::NumberToString(it.second.count) + "\n";
            // total += line;
            const AgentMetricStatus &item = it.second;
            ss << ToSeconds(item.timestamp) << "|" << item.metricName << "|" << item.count << std::endl;
        }
        return FileUtils::WriteFileContent(file, ss.str());
    }

    int AgentStatus::UpdateAgentStatus(AGENT_STATUS_TYPE type) {
        Sync(mMetricMap) {
            if (mMetricMap.find(type) == mMetricMap.end()) {
                // mMetricMapLock.unlock();
                LogWarn("unsupported type {}", static_cast<int>(type));
                return -1;
            }
            // int64_t now = NowSeconds();
            auto now = std::chrono::Now<std::chrono::seconds>();
            if (mMetricMap[type].timestamp + mEffectInterval >= now) {
                //有效的数据
                mMetricMap[type].count++;
                mMetricMap[type].timestamp = now;
            } else {
                //无效的
                mMetricMap[type].count = 1;
                mMetricMap[type].timestamp = now;
            }
            SaveAgentStatus(mAgentStatusFile, mMetricMap);
            return 0;
        }}}
        // mMetricMapLock.unlock();
        // return 0;
    }

    bool AgentStatus::GetAgentStatus(AgentMetricStatus *status) const {
        // int64_t now = NowSeconds();
        // mMetricMapLock.lock();
        // if (mMetricMap[AGENT_RESOURCE_STATUS].timestamp + mEffectInterval > now &&
        //     mMetricMap[AGENT_RESOURCE_STATUS].count >= mMaxResouceCount) {
        //     mMetricMapLock.unlock();
        //     return false;
        // }
        // if (mMetricMap[AGENT_COREDUMP_STATUS].timestamp + mEffectInterval > now &&
        //     mMetricMap[AGENT_COREDUMP_STATUS].count >= mMaxCoredumpCount) {
        //     mMetricMapLock.unlock();
        //     return false;
        // }
        // /*
        // if(mMetricMap[AGENT_START_STATUS].timestamp+mEffectInterval>now&&mMetricMap[AGENT_START_STATUS].count>=mMaxStartCount)
        // {
        //     return false;
        // }*/
        // mMetricMapLock.unlock();
        // return true;

        const auto minTime = std::chrono::Now<std::chrono::seconds>() - mEffectInterval;
        Sync(mMetricMap) {
            for (const auto &it: mMetricMap) {
                const auto &v = it.second;
                if (it.first != AGENT_START_STATUS && (v.timestamp >= minTime && v.count >= v.maxCount)) {
					if (status) {
						*status = v;
					}
                    return false;
                }
            }
            return true;
        }}}
    }

    int AgentStatus::GetAgentStatus(AGENT_STATUS_TYPE type) const {
        const auto minTime = std::chrono::Now<std::chrono::seconds>() - mEffectInterval;
        Sync(mMetricMap) {
            int count = 0;
            auto it = mMetricMap.find(type);
            if (it != mMetricMap.end() && it->second.timestamp >= minTime) {
                count = it->second.count;
            }
            return count;
        }}}
        // mMetricMapLock.lock();
        // if (mMetricMap.find(type) == mMetricMap.end()) {
        //     mMetricMapLock.unlock();
        //     LogWarn("unsuported type {}", static_cast<int>(type));
        //     return 0;
        // }
        // int count = 0;
        // int64_t now = NowSeconds();
        // if (mMetricMap[type].timestamp + mEffectInterval >= now) {
        //     count = mMetricMap[type].count;
        // }
        // mMetricMapLock.unlock();
        // return count;
    }
}