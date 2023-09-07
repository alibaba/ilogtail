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

#include "AdhocCheckpointManager.h"
#include "common/Flags.h"
#include "common/Thread.h"
#include "logger/Logger.h"

DEFINE_FLAG_INT32(adhoc_checkpoint_dump_thread_wait_interval, "microseconds", 5 * 1000);

namespace logtail {

void AdhocCheckpointManager::Run() {
    if (mRunFlag) {
        return;
    } else {
        mRunFlag = true;
    }
        
    new Thread([this]() { ProcessLoop(); });
    LOG_INFO(sLogger, ("AdhocCheckpointManager", "Start"));
}

void AdhocCheckpointManager::ProcessLoop() {
    int32_t prevTime = time(NULL);
    int32_t curTime = prevTime;
    int32_t interval;

    while (true) {
        // dump checkpoint per 5s
        curTime = time(NULL);
        interval = INT32_FLAG(adhoc_checkpoint_dump_thread_wait_interval) - (curTime - prevTime);
        usleep(interval);

        for (auto& p : mAdhocJobCheckpointMap) {
            p.second->DumpAdhocCheckpoint();
        }
    }

    LOG_WARNING(sLogger, ("AdhocCheckpointManager", "Exit"));
}

AdhocJobCheckpointPtr AdhocCheckpointManager::GetAdhocJobCheckpoint(const std::string& jobName) {
    auto it = mAdhocJobCheckpointMap.find(jobName);
    if (it != mAdhocJobCheckpointMap.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

AdhocJobCheckpointPtr AdhocCheckpointManager::CreateAdhocJobCheckpoint(const std::string& jobName, std::vector<AdhocFileCheckpointKey> adHocFileCheckpointKeyList) {
    AdhocJobCheckpointPtr adHocJobCheckpointPtr = GetAdhocJobCheckpoint(jobName);
    if (nullptr == adHocJobCheckpointPtr) {
        adHocJobCheckpointPtr = std::make_shared<AdhocJobCheckpoint>(jobName);
        for (AdhocFileCheckpointKey key : adHocFileCheckpointKeyList) {
            adHocJobCheckpointPtr->AddAdhocFileCheckpoint(key);
        }
        mAdhocJobCheckpointMap[jobName] = adHocJobCheckpointPtr;

        LOG_INFO(sLogger, ("Create AdhocJobCheckpoint success, job name", jobName));
        return adHocJobCheckpointPtr;
    } else {
        LOG_INFO(sLogger, ("Create AdhocJobCheckpoint failed, job checkpoint already exists, job name", jobName));
        return adHocJobCheckpointPtr;
    }
}

void AdhocCheckpointManager::DeleteAdhocJobCheckpoint(const std::string& jobName) {
    auto it = mAdhocJobCheckpointMap.find(jobName);
    if (it != mAdhocJobCheckpointMap.end()) {
        it->second->Delete();
    } else {
        LOG_WARNING(sLogger, ("Delete AdhocJobCheckpoint fail, job checkpoint doesn't exist, job name", jobName));
    }
}

void AdhocCheckpointManager::LoadAdhocCheckpoint() {
    
}

} // namespace logtail