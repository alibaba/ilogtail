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

typedef std::shared_ptr<std::vector<AdhocFileCheckpointPtr> > AdhocFileCheckpointListPtr;

struct AdhocFileReaderKey {
    AdhocFileReaderKey() {}
    AdhocFileReaderKey(const std::string& jobName, const AdhocFileKey& fileKey)
        : mJobName(jobName), mFileKey(fileKey) {}
    std::string mJobName;
    AdhocFileKey mFileKey;
};

enum AdhocEventType {
    EVENT_START_JOB,
    EVENT_READ_FILE,
    EVENT_STOP_JOB,
};

class AdhocEvent {
private:
    void FindJobByName();

    AdhocEventType mType;
    AdhocFileReaderKey mReaderKey;
    std::shared_ptr<Config> mJobConfig;
    AdhocFileCheckpointListPtr mFileCheckpointList;
    int32_t mWaitTimes;

public:
    AdhocEvent() {}
    // for start job
    AdhocEvent(AdhocEventType eventType, const std::string& jobName, AdhocFileCheckpointListPtr fileCheckpointList)
        : mType(eventType), mReaderKey(AdhocFileReaderKey(jobName, AdhocFileKey())), mFileCheckpointList(fileCheckpointList) {}
    // for stop job
    AdhocEvent(AdhocEventType eventType, const std::string& jobName)
        : mType(eventType), mReaderKey(AdhocFileReaderKey(jobName, AdhocFileKey())) {}
    // for read file
    AdhocEvent(AdhocEventType eventType, const AdhocFileReaderKey& readerKey)
        : mType(eventType), mReaderKey(readerKey) {
        mWaitTimes = 0;
        FindJobByName();
    }

    inline AdhocEventType GetType() { return mType; }
    inline std::string GetJobName() { return mReaderKey.mJobName; }
    inline AdhocFileKey* GetAdhocFileKey() { return &mReaderKey.mFileKey; }
    inline AdhocFileReaderKey* GetAdhocFileReaderKey() { return &mReaderKey; }
    inline std::shared_ptr<Config> GetJobConfig() { return mJobConfig; }
    inline AdhocFileCheckpointListPtr GetFileCheckpointList() { return mFileCheckpointList; }
    inline int32_t IncreaseWaitTimes() { return ++mWaitTimes; }
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

    static bool mRunFlag;
    AdhocCheckpointManager* mAdhocCheckpointManager;
    static const int32_t ADHOC_JOB_MAX = 100 * 2;
    CircularBufferSem<AdhocEvent*, ADHOC_JOB_MAX> mEventQueue;
    std::unordered_set<std::string> mDeletedJobSet;
    std::unordered_map<std::string, std::vector<AdhocFileKey> > mJobFileKeyListMap;
    std::unordered_map<AdhocFileReaderKey*, LogFileReaderPtr> mAdhocFileReaderMap;

public:
    static AdhocFileManager* GetInstance() {
        static AdhocFileManager* ptr = new AdhocFileManager();
        return ptr;
    }

    void AddJob(const std::string& jobName, const std::vector<std::string>& filePathList);
    void DeleteJob(const std::string& jobName);
};

} // namespace logtail
