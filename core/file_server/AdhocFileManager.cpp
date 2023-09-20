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

#include "AdhocFileManager.h"
#include "logger/Logger.h"
#include "common/Thread.h"
#include "config_manager/ConfigManager.h"
#include "reader/LogFileReader.h"

namespace logtail {

AdhocEventType AdhocEvent::GetType() {
    return mType;
}

std::string AdhocEvent::GetJobName() {
    return mReaderKey->mJobName;
}

AdhocFileKey* AdhocEvent::GetAdhocFileKey() {
    return mReaderKey->mFileKey;
}

AdhocFileReaderKey* AdhocEvent::GetAdhocFileReaderKey() {
    return mReaderKey;
}

std::shared_ptr<Config> AdhocEvent::GetJobConfig() {
    FindJobByName();
    return mJobConfig;
}

void AdhocEvent::SetConfigName(std::string jobName) {
    mReaderKey->mJobName = jobName;
}

void AdhocEvent::FindJobByName() {
    Config* pConfig = ConfigManager::GetInstance()->FindConfigByName(GetJobName());
    mJobConfig.reset(new Config(*pConfig));
}

AdhocFileManager::AdhocFileManager() {
    mRunFlag = false;
};

void AdhocFileManager::Run() {
    if (mRunFlag) {
        return;
    } else {
        mRunFlag = true;
    }

    new Thread([this]() { ProcessLoop(); });
    LOG_INFO(sLogger, ("AdhocFileManager", "Start"));
}

void AdhocFileManager::ProcessLoop() {
    mAdhocCheckpointManager = AdhocCheckpointManager::GetInstance();

    while (mRunFlag) {
        AdhocEvent* ev = PopEventQueue();
        if (NULL == ev) {
            LOG_INFO(sLogger, ("AdhocFileManager", "All file loaded, exit"));
            mRunFlag = false;
            break;
        }
        switch (ev->GetType()) {
            case EVENT_READ_FILE:
                ProcessReadFileEvent(ev);
                break;
            case EVENT_STOP_JOB:
                ProcessStopJobEvent(ev);
                break;
        }
    }
    LOG_INFO(sLogger, ("AdhocFileManager", "Stop"));
}

void AdhocFileManager::ProcessReadFileEvent(AdhocEvent* ev) {
    std::string jobName = ev->GetJobName();
    AdhocFileKey* fileKey = ev->GetAdhocFileKey();
    AdhocFileReaderKey* fileReaderKey = ev->GetAdhocFileReaderKey();
    AdhocJobCheckpointPtr jobCheckpoint = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName);
    AdhocFileCheckpointPtr fileCheckpoint = mAdhocCheckpointManager->GetAdhocFileCheckpoint(jobName, fileKey);

    // read file and change offset
    ReadFile(ev, fileCheckpoint);

    // if file's status changed, dump job's checkpoint
    mAdhocCheckpointManager->UpdateAdhocFileCheckpoint(jobName, fileKey, fileCheckpoint);

    // Push file of this job into queue
    int32_t nowFileIndex = jobCheckpoint->GetCurrentFileIndex();
    if (nowFileIndex < mJobFileKeyLists[jobName].size()) {
        AdhocFileKey* newFileKey = mJobFileKeyLists[jobName][nowFileIndex];
        AdhocEvent* newEv = new AdhocEvent(EVENT_READ_FILE, ev->GetAdhocFileReaderKey());
        PushEventQueue(newEv);
    }
}

void AdhocFileManager::ReadFile(AdhocEvent* ev, AdhocFileCheckpointPtr ptr) {
    // if (ptr->mStartTime == 0) {
    //     ptr->mStartTime = time(NULL);
    // }
    // if (ev->GetInode() == ptr->mDevInode.inode) { // check
    //     // read
    //     // ptr->mOffset += 1024;
    // } else {
    //     // file cannot find
    //     ptr->mOffset = -1;
    // }
}

void AdhocFileManager::ProcessStopJobEvent(AdhocEvent* ev) {
    std::string jobName = ev->GetJobName();
    mDeletedJobSet.insert(jobName);
    mAdhocCheckpointManager->DeleteAdhocJobCheckpoint(jobName);
}

void AdhocFileManager::PushEventQueue(AdhocEvent* ev) {
    mEventQueue.push(ev);
}

AdhocEvent* AdhocFileManager::PopEventQueue() {
    while (mEventQueue.size() > 0) {
        AdhocEvent* ev = mEventQueue.front();
        mEventQueue.pop();

        std::string jobName = ev->GetJobName();
        if (mDeletedJobSet.find(jobName) != mDeletedJobSet.end()) {
            continue;
        }

        return ev;
    }
    return NULL;
}

void AdhocFileManager::AddJob(const std::string& jobName, std::vector<std::string> filePathList) {
    std::vector<AdhocFileCheckpointPtr> fileCheckpointList;
    std::vector<AdhocFileKey*> fileKeyList;
    for (std::string filePath : filePathList) {
        AdhocFileCheckpointPtr fileCheckpoint = mAdhocCheckpointManager->CreateAdhocFileCheckpoint(jobName, filePath);
        if (nullptr != fileCheckpoint) {
            AdhocFileKey fileKey(
                fileCheckpoint->mDevInode, fileCheckpoint->mSignatureSize, fileCheckpoint->mSignatureHash);
            fileKeyList.push_back(&fileKey);
            fileCheckpointList.push_back(fileCheckpoint);
        }
    }

    AdhocJobCheckpointPtr jobCheckpoint
        = mAdhocCheckpointManager->CreateAdhocJobCheckpoint(jobName, fileCheckpointList);
    if (nullptr != jobCheckpoint) {
        mJobFileKeyLists[jobName] = fileKeyList;

        // push first file to queue
        if (fileKeyList.size() > 0) {
            AdhocEvent* ev = new AdhocEvent(EVENT_READ_FILE, &AdhocFileReaderKey(jobName, fileKeyList[0]));
            PushEventQueue(ev);
        }

        Run();
    }
}

void AdhocFileManager::DeleteJob(const std::string& jobName) {
    mJobFileKeyLists.erase(jobName);
    AdhocEvent* stopEvent = new AdhocEvent(EVENT_STOP_JOB);
    stopEvent->SetConfigName(jobName);
    PushEventQueue(stopEvent);
}

} // namespace logtail
