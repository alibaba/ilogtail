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
#include <queue>
#include <unordered_set>
#include "config/Config.h"
#include "checkpoint/AdhocCheckpointManager.h"
#include "reader/LogFileReader.h"

namespace logtail {

struct AdhocFileReaderKey {
    AdhocFileReaderKey() {}
    AdhocFileReaderKey(std::string jobName, AdhocFileKey fileKey) : mJobName(jobName), mFileKey(fileKey) {}
    std::string mJobName;
    AdhocFileKey mFileKey;
};

enum AdhocEventType {
    EVENT_READ_FILE,
    EVENT_STOP_JOB,
};

class AdhocEvent {
private:
    void FindJobByName();

    AdhocEventType mType;
    AdhocFileReaderKey mReaderKey;
    std::shared_ptr<Config> mJobConfig;

public:
    AdhocEvent() {}
    AdhocEvent(AdhocEventType eventType) : mType(eventType) {}
    AdhocEvent(AdhocEventType eventType, AdhocFileReaderKey readerKey) : mType(eventType), mReaderKey(readerKey) {}

    AdhocEventType GetType();
    std::string GetJobName();
    AdhocFileKey* GetAdhocFileKey();
    AdhocFileReaderKey* GetAdhocFileReaderKey();
    std::shared_ptr<Config> GetJobConfig();

    void SetConfigName(std::string jobName);
};

class AdhocFileManager {
private:
    AdhocFileManager();
    AdhocFileManager(const AdhocFileManager&) = delete;
    AdhocFileManager& operator=(const AdhocFileManager&) = delete;
    void ProcessLoop();
    static bool mRunFlag;

    void PushEventQueue(AdhocEvent* ev);
    AdhocEvent* PopEventQueue();
    void ProcessReadFileEvent(AdhocEvent* ev);
    void ProcessStopJobEvent(AdhocEvent* ev);

    AdhocCheckpointManager* mAdhocCheckpointManager;
    std::queue<AdhocEvent*> mEventQueue;
    std::unordered_set<std::string> mDeletedJobSet;
    std::unordered_map<std::string, std::vector<AdhocFileKey> > mJobFileKeyLists;
    std::unordered_map<AdhocFileReaderKey*, LogFileReaderPtr> mAdhocFileReaderMap;

public:
    static AdhocFileManager* GetInstance() {
        static AdhocFileManager* ptr = new AdhocFileManager();
        return ptr;
    }

    void Run();
    void AddJob(const std::string& jobName, std::vector<std::string> filePathList);
    void DeleteJob(const std::string& jobName);
};

} // namespace logtail