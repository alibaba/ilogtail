/*
 * Copyright 2023 iLogtail Authors
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
#include <unordered_set>
#include "config/Config.h"
#include "checkpoint/AdhocCheckpointManager.h"
#include "reader/LogFileReader.h"
#include "common/CircularBuffer.h"

namespace logtail {

struct AdhocFileReaderKey {
    AdhocFileReaderKey() {}
    AdhocFileReaderKey(const std::string& jobName,
                       const DevInode& devInode,
                       uint32_t signatureSize,
                       uint64_t signatureHash)
        : mJobName(jobName), mDevInode(devInode), mSignatureSize(signatureSize), mSignatureHash(signatureHash) {}
    std::string mJobName;
    DevInode mDevInode;
    uint32_t mSignatureSize;
    uint64_t mSignatureHash;

    inline std::string ToString() {
        return mJobName + std::to_string(mDevInode.dev) + std::to_string(mDevInode.inode)
            + std::to_string(mSignatureSize) + std::to_string(mSignatureHash);
    }
};

enum AdhocEventType {
    EVENT_START_JOB,
    EVENT_READ_FILE,
    EVENT_STOP_JOB,
};

struct AdhocEvent {
    AdhocEvent() {}
    // for start job
    AdhocEvent(AdhocEventType eventType, const std::string& jobName, std::vector<std::string>& filePathList)
        : mType(eventType), mJobName(jobName), mFilePathList(filePathList) {}
    // for stop job
    AdhocEvent(AdhocEventType eventType, const std::string& jobName) : mType(eventType), mJobName(jobName) {}
    // for read file
    AdhocEvent(AdhocEventType eventType, const std::string& jobName, int32_t waitTimes)
        : mType(eventType), mJobName(jobName), mWaitTimes(waitTimes) {}

    AdhocEventType mType;
    std::string mJobName;
    int32_t mWaitTimes;
    std::vector<std::string> mFilePathList;
};

class AdhocFileManager {
private:
    AdhocFileManager();
    AdhocFileManager(const AdhocFileManager&) = delete;
    AdhocFileManager& operator=(const AdhocFileManager&) = delete;
    void Run();
    void ProcessLoop();

    void PushEventQueue(AdhocEvent* ev);
    AdhocEvent* PopEventQueue();
    void ProcessStartJobEvent(AdhocEvent* ev);
    void ProcessReadFileEvent(AdhocEvent* ev);
    void ProcessStopJobEvent(AdhocEvent* ev);

    Config* GetJobConfig(const std::string& jobName);

    static bool mRunFlag;
    AdhocCheckpointManager* mAdhocCheckpointManager;
    static const int32_t ADHOC_JOB_MAX = 100 * 2;
    CircularBufferSem<AdhocEvent*, ADHOC_JOB_MAX> mEventQueue;
    std::unordered_set<std::string> mDeletedJobSet;
    std::unordered_map<std::string, LogFileReaderPtr> mAdhocFileReaderMap;

public:
    static AdhocFileManager* GetInstance() {
        static AdhocFileManager* ptr = new AdhocFileManager();
        return ptr;
    }

    void AddJob(const std::string& jobName, std::vector<std::string>& filePathList);
    void DeleteJob(const std::string& jobName);
};

} // namespace logtail
