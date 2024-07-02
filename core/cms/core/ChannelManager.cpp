#include "ChannelManager.h"

#include <utility>
#include "common/Logger.h"
#include "common/ScopeGuard.h"

using namespace std;
using namespace common;

namespace argus {
    ChannelManager::ChannelManager() = default;

    ChannelManager::~ChannelManager() = default;

    void ChannelManager::Start() {
        if (!mIsStart) {
            mIsStart = true;

            Sync(mChannelMap) {
                for (auto &it: mChannelMap) {
                    it.second->runIt();
                }
            }}}
        }
    }

    void ChannelManager::End() {
        mChannelMap.Clear();
    }

    std::shared_ptr<OutputChannel> ChannelManager::GetOutputChannel(const std::string &name) const {
        std::shared_ptr<OutputChannel> ret;
        mChannelMap.Find(name, ret);
        return ret;
    }

    void ChannelManager::AddOutputChannel(const std::string &name, std::shared_ptr<OutputChannel> pChannel) {
        mChannelMap.MustSet(name, pChannel);
    }

    int SendToChannel(const ChannelManager &cm, const std::vector<ChannelManager::ChanConf> &outputs,
                      const std::function<void(OutputChannel &, const std::string &)> &send) {
        int count = 0;
        for (const auto &i: outputs) {
            const string &chanName = i.first;
            auto pChannel = cm.GetOutputChannel(chanName);
            if (pChannel == nullptr) {
                LogError("unknown channel({})", chanName);
            } else {
                count++;
                const string &conf = i.second;
                send(*pChannel, conf);
            }
        }

        return count;
    }

    int ChannelManager::sendMsgToNextModule(const string &name, const std::vector<ChanConf> &outputVector,
                                            const std::chrono::system_clock::time_point &timestamp,
                                            int exitCode, const string &result, bool reportStatus,
                                            const string &mid) const {
        return SendToChannel(*this, outputVector, [&](OutputChannel &chan, const std::string &conf) {
            chan.addMessage(name, timestamp, exitCode, result, conf, reportStatus, mid);
        });
        // // bool saSend = false;
        // for(const auto & output : outputVector){
        //     string channelName=output.first;
        //     string conf=output.second;
        //     //LogDebug("channelName({}),conf={}",channelName.c_str(),conf.c_str());
        //     auto pChannel = GetOutputChannel(channelName);
        //     if(pChannel == nullptr){
        //         LogError("unknown channel({}), name={}", channelName, name);
        //         continue;
        //     }
        //     // if(channelName=="staragent"){
        //     //     saSend = true;
        //     // }
        //     pChannel->addMessage(name, timestamp, exitCode, result, conf, reportStatus, mid);
        // }
        // // //如果需要上报状态，则需要进一步上报状态
        // // if(saSend==false && reportStatus ==true){
        // //     sendStatusToNextModule(name,timestamp,exitCode,"","exec");
        // // }
    }
// void ChannelManager::sendStatusToNextModule(std::string name,int timestamp,int dealCode,string dealResult,string type){
//     #ifndef WIN32
//     SaChannel * pChannel=(SaChannel *)GetOutputChannel("staragent");
//     if(pChannel!=NULL)
//     {
//         pChannel->addStatus(name,timestamp,dealCode,dealResult,type);
//     }
//     #endif
// }

    template<typename TData>
    int Send(const ChannelManager &cm, const TData &data, const std::vector<ChannelManager::ChanConf> &outputs,
             int (OutputChannel::*Send)(const TData &, std::string)) {
        return SendToChannel(cm, outputs, [&](OutputChannel &chan, const std::string &conf) {
            (chan.*Send)(data, conf);
        });
        // int count = 0;
        // for (const auto &i: outputs) {
        //     const string &chanName = i.first;
        //     auto pChannel = cm.GetOutputChannel(chanName);
        //     if (pChannel == nullptr) {
        //         LogError("unknown channel({})", chanName.c_str());
        //     } else {
        //         count++;
        //         const string &conf = i.second;
        //         (pChannel.get()->*Send)(data, conf);
        //     }
        // }
        //
        // return count;
    }

    int ChannelManager::SendMetricToNextModule(const CommonMetric &metric,
                                               const std::vector<ChanConf> &outputVector) const {
        return Send(*this, metric, outputVector, &OutputChannel::AddCommonMetric);
        // int count = 0;
        // for (const auto & i : outputVector) {
        // 	string channelName = i.first;
        //     string conf=i.second;
        //     auto pChannel=GetOutputChannel(channelName);
        //     if(pChannel == nullptr){
        //         LogError("unknown channel({})",channelName.c_str());
        //         continue;
        //     }
        //     pChannel->AddCommonMetric(metric,conf);
        // }
    }

    int ChannelManager::SendMetricsToNextModule(const std::vector<CommonMetric> &metrics,
                                                const std::vector<ChanConf> &outputVector) const {
        return Send(*this, metrics, outputVector, &OutputChannel::AddCommonMetrics);
        // int count = 0;
        // for (const auto & output : outputVector) {
        //     string channelName=output.first;
        //     string conf=output.second;
        //     auto pChannel=GetOutputChannel(channelName);
        //     if(pChannel == nullptr){
        //         LogError("unknown channel({})",channelName.c_str());
        //         continue;
        //     }
        //     count++;
        //     pChannel->AddCommonMetrics(metrics,conf);
        // }
        // return count;
    }

    typedef std::unordered_map<std::string, std::string> CResultMap;

    int ChannelManager::SendResultMapToNextModule(const std::vector<CResultMap> &resultMaps,
                                                  const std::vector<ChanConf> &outputVector) const {
        return Send(*this, resultMaps, outputVector, &OutputChannel::AddResultMap);
        // int count = 0;
        // for(const auto & output : outputVector)
        // {
        //     string channelName=output.first;
        //     string conf=output.second;
        //     auto pChannel=GetOutputChannel(channelName);
        //     if(pChannel==nullptr)
        //     {
        //         LogError("unknown channel({})",channelName.c_str());
        //         continue;
        //     }
        //     count++;
        //     pChannel->AddResultMap(resultMaps,conf);
        // }
        // return count;
    }

    int ChannelManager::SendRawDataToNextModule(const RawData &rawData,
                                                const std::vector<ChanConf> &outputVector) const {
        return Send(*this, rawData, outputVector, &OutputChannel::AddRawData);
        // for (auto &i: outputVector) {
        //     string channelName = i.first;
        //     auto pChannel = GetOutputChannel(channelName);
        //     if (pChannel == nullptr) {
        //         LogError("unknown channel({})", channelName.c_str());
        //     } else {
        //         string conf = i.second;
        //         pChannel->AddRawData(rawData, conf);
        //     }
        // }
    }

    bool ChannelManager::GetOutputChannelStatus(const std::string &name, CommonMetric &metric) const {
        auto pChannel = GetOutputChannel(name);
        if (pChannel) {
            pChannel->GetChannelStatus(metric);
        }
        return (bool) pChannel;
    }
}