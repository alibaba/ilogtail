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
#include "common/FileSystemUtil.h"
#include "processor/LogProcess.h"

namespace logtail {

void AdhocEvent::FindJobByName() {
    Config* pConfig = ConfigManager::GetInstance()->FindConfigByName(GetJobName());
    mJobConfig.reset(new Config(*pConfig));
}

AdhocFileManager::AdhocFileManager() {
    mRunFlag = false;
}

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
            case EVENT_START_JOB:
                ProcessStartJobEvent(ev);
                break;
            case EVENT_READ_FILE:
                ProcessReadFileEvent(ev);
                break;
            case EVENT_STOP_JOB:
                ProcessStopJobEvent(ev);
                break;
        }

        mAdhocCheckpointManager->DumpAdhocCheckpoint();
    }
    LOG_INFO(sLogger, ("AdhocFileManager", "Stop"));
}

void AdhocFileManager::ProcessStartJobEvent(AdhocEvent* ev) {
    std::string jobName = ev->GetJobName();
    AdhocFileCheckpointListPtr fileCheckpointList = ev->GetFileCheckpointList();

    AdhocJobCheckpointPtr jobCheckpoint
        = mAdhocCheckpointManager->CreateAdhocJobCheckpoint(jobName, *fileCheckpointList);
    // makesure jobCheckpoint exists and doesn't have fileKeyList
    if (nullptr != jobCheckpoint && mJobFileKeyListMap.find(jobName) == mJobFileKeyListMap.end()) {
        std::vector<AdhocFileKey> fileKeyList;
        for (AdhocFileCheckpointPtr fileCheckpoint : *fileCheckpointList) {
            AdhocFileKey fileKey(
                fileCheckpoint->mDevInode, fileCheckpoint->mSignatureSize, fileCheckpoint->mSignatureHash);
            fileKeyList.push_back(fileKey);
        }
        mJobFileKeyListMap[jobName] = fileKeyList;

        // push first file to queue
        if (fileKeyList.size() > 0) {
            AdhocEvent* newEv = new AdhocEvent(EVENT_READ_FILE, AdhocFileReaderKey(jobName, fileKeyList[0]));
            PushEventQueue(newEv);
        }
    }
}

void AdhocFileManager::ProcessReadFileEvent(AdhocEvent* ev) {
    std::string jobName = ev->GetJobName();
    AdhocFileKey* fileKey = ev->GetAdhocFileKey();
    AdhocFileReaderKey* fileReaderKey = ev->GetAdhocFileReaderKey();
    AdhocJobCheckpointPtr jobCheckpoint = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName);
    AdhocFileCheckpointPtr fileCheckpoint = mAdhocCheckpointManager->GetAdhocFileCheckpoint(jobName, fileKey);

    // read file and change offset
    if (mAdhocFileReaderMap.find(fileReaderKey) == mAdhocFileReaderMap.end()) {
        std::string dirPath = ParentPath(fileCheckpoint->mRealFileName);
        std::string fileName = fileCheckpoint->mRealFileName.substr(dirPath.length());
        LogFileReaderPtr readerSharePtr(
            ev->GetJobConfig()->CreateLogFileReader(dirPath, fileName, fileKey->mDevInode, true));
        mAdhocFileReaderMap[fileReaderKey] = readerSharePtr;
    }

    LogFileReaderPtr fileReader = mAdhocFileReaderMap[fileReaderKey];
    if (!LogProcess::GetInstance()->IsValidToReadAdhocLog(fileReader->GetLogstoreKey())) {
        // Log warning and send alarm per 10s( >10s if multi jobs exist)
        if (0 == ev->IncreaseWaitTimes() % 1000) {
            LOG_WARNING(sLogger,
                        ("read adhoc file failed, logprocess queue is full, put adhoc event to event queue again",
                         fileReader->GetHostLogPath())(fileReader->GetProjectName(), fileReader->GetCategory()));
            LogtailAlarm::GetInstance()->SendAlarm(
                PROCESS_QUEUE_BUSY_ALARM,
                std::string(
                    "read adhoc file failed, logprocess queue is full, put adhoc event to event queue again, file:")
                    + fileReader->GetHostLogPath() + " ,project:" + fileReader->GetProjectName()
                    + " ,logstore:" + fileReader->GetCategory());
        }

        PushEventQueue(ev);
        usleep(1000 * 10);
        return;
    }
    LogBuffer* logBuffer = new LogBuffer;
    fileReader->ReadLog(*logBuffer, nullptr);
    if (!logBuffer->rawBuffer.empty()) {
        logBuffer->logFileReader = fileReader;
        LogProcess::GetInstance()->PushBuffer(logBuffer, 100000000);
        fileCheckpoint->mOffset = fileReader->GetLastFilePos();
        fileCheckpoint->mRealFileName = fileReader->GetHostLogPathFile();
    } else {
        delete logBuffer;
    }
    mAdhocCheckpointManager->UpdateAdhocFileCheckpoint(jobName, fileKey, fileCheckpoint);

    // Push file of this job into queue
    int32_t nowFileIndex = jobCheckpoint->GetCurrentFileIndex();
    if (nowFileIndex < mJobFileKeyListMap[jobName].size()) {
        AdhocEvent* newEv
            = new AdhocEvent(EVENT_READ_FILE, AdhocFileReaderKey(jobName, mJobFileKeyListMap[jobName][nowFileIndex]));
        PushEventQueue(newEv);
    }
}

void AdhocFileManager::ProcessStopJobEvent(AdhocEvent* ev) {
    std::string jobName = ev->GetJobName();
    mDeletedJobSet.insert(jobName);
    mJobFileKeyListMap.erase(jobName);
    mAdhocCheckpointManager->DeleteAdhocJobCheckpoint(jobName);
}

void AdhocFileManager::PushEventQueue(AdhocEvent* ev) {
    mEventQueue.PushItem(ev);
}

AdhocEvent* AdhocFileManager::PopEventQueue() {
    while (mEventQueue.GetItemNumber() > 0) {
        AdhocEvent* ev;
        mEventQueue.PopItem(ev);

        std::string jobName = ev->GetJobName();
        if (mDeletedJobSet.find(jobName) != mDeletedJobSet.end()) {
            continue;
        }
        return ev;
    }
    return NULL;
}

void AdhocFileManager::AddJob(const std::string& jobName, const std::vector<std::string>& filePathList) {
    AdhocFileCheckpointListPtr fileCheckpointList = std::make_shared<std::vector<AdhocFileCheckpointPtr> >();
    for (std::string filePath : filePathList) {
        AdhocFileCheckpointPtr fileCheckpoint = mAdhocCheckpointManager->CreateAdhocFileCheckpoint(jobName, filePath);
        if (nullptr != fileCheckpoint) {
            fileCheckpointList->push_back(fileCheckpoint);
        }
    }
    AdhocEvent* ev = new AdhocEvent(EVENT_START_JOB, jobName, fileCheckpointList);
    PushEventQueue(ev);
    Run();
}

void AdhocFileManager::DeleteJob(const std::string& jobName) {
    AdhocEvent* stopEvent = new AdhocEvent(EVENT_STOP_JOB, jobName);
    PushEventQueue(stopEvent);
}

} // namespace logtail
