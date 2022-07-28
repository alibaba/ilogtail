/*
 * Copyright 2022 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "interface/network.h"
#include "interface/helper.h"
#include "NetworkConfig.h"
#include <unordered_map>
#include <ostream>
#include "log_pb/sls_logs.pb.h"
#include "common/Thread.h"
#include "common/Lock.h"
#include "common/TimeUtil.h"
#include "common/StringPiece.h"
#include "metas/ContainerProcessGroup.h"
#include "ConnectionObserver.h"
#include "metas/ConnectionMetaManager.h"


namespace logtail {
class ProcessObserver;
class PCAPWrapper;
class EBPFWrapper;


class NetworkObserver {
public:
    NetworkObserver() {
        mLastGCTimeNs = mLastFlushTimeNs = GetCurrentTimeInNanoSeconds();
        mConfig = NetworkConfig::GetInstance();
        mNetworkStatistic = NetworkStatistic::GetInstance();
        mServiceMetaManager = ServiceMetaManager::GetInstance();
    }

    ~NetworkObserver();

    static NetworkObserver* GetInstance() {
        static NetworkObserver* sObserver = new NetworkObserver();
        return sObserver;
    }

    void EventLoop();

    /**
     * @brief do not used yet.
     *
     */
    void Stop() {
        if (mEventLoopThread) {
            mEventLoopThread->Wait(100);
        }
    }

    void HoldOn(bool exitFlag = false);

    void Resume();

    void Reload();

    void BindSender();

    static int SendToPluginProcessed(std::vector<sls_logs::Log>& logs, Config* cfg);


    void OnProcessDestroyed(uint32_t pid, const char* command, size_t len);

    ProcessObserver* GetProcess(PacketEventHeader* header, bool create = true);

    int OnPacketEventStringPiece(StringPiece sp) { return OnPacketEvent((void*)sp.c_str(), sp.size()); }

    int OnPacketEvent(void* event, size_t len);

    /**
     * @brief FlushOut 其实和NetworkObserver没啥关系，直接从ContainerProcessGroupManager就可以获取所有的Aggregator和Meta
     *
     * @param allData
     */
    void FlushOutMetrics(std::vector<sls_logs::Log>& allData);

    /**
     * @brief 输出4层的统计信息
     *
     * @param allData
     */
    void FlushOutStatistics(std::vector<sls_logs::Log>& allData);

    /**
     * @brief 复用FlushOutMetrics的能力
     *
     * @return std::string
     */
    std::string FlushToString() {
        std::vector<sls_logs::Log> allData;
        FlushOutMetrics(allData);
        std::string rst;
        for (auto& log : allData) {
            rst.append(log.DebugString()).append("\n");
        }
        return rst;
    }

    void SetUpSender(const std::function<int(std::vector<sls_logs::Log>&, Config*)>& senderFunc) {
        mSenderFunc = senderFunc;
    }

    void GarbageCollection(uint64_t nowTimeNs);

    uint32_t EBPFConnectionGC(uint64_t nowTimeNs);

    friend class NetworkObserverUnittest;
    friend class PCAPWrapperUnittest;
    friend class EBPFWrapperUnittest;
    friend class LocalFileWrapperUnittest;
    friend class ProtocolDnsUnittest;
    friend class ProtocolHttpUnittest;
    friend class ProtocolMySqlUnittest;
    friend class ProtocolRedisUnittest;
    friend class ProtocolPgSqlUnittest;

protected:
    void StartEventLoop() {
        if (!mEventLoopThread) {
            mEventLoopThread = CreateThread([this]() { EventLoop(); });
        }
    }

    void ReloadSource();

    void FlushStatistics(NetStaticticsMap& map, std::vector<sls_logs::Log>& logs);

protected:
    // key : pid << 32 | sockHash
    std::unordered_map<uint32_t, ProcessObserver*> mAllProcesses;
    NetworkConfig* mConfig;
    NetworkStatistic* mNetworkStatistic;
    ServiceMetaManager* mServiceMetaManager;

    std::function<int(std::vector<sls_logs::Log>&, Config*)> mSenderFunc;

    ThreadPtr mEventLoopThread;
    ReadWriteLock mEventLoopThreadRWL;
    uint64_t mLastGCTimeNs = 0;
    uint64_t mLastFlushTimeNs = 0;
    uint64_t mLastEbpfGCTimeNs = 0;
    uint64_t mLastFlushMetaTimeNs = 0;
    uint64_t mLastFlushNetlinkTimeNs = 0;
    uint64_t mLastProbeDisableProcessNs = 0;
    uint64_t mLastCleanAllDisableProcessNs = 0;
    PCAPWrapper* mPCAPWrapper = NULL;
    EBPFWrapper* mEBPFWrapper = NULL;
    FILE* mDumpFilePtr = NULL;
    FILE* mReplayFilePtr = NULL;
    int64_t mDumpSize = 0;
};

} // namespace logtail
