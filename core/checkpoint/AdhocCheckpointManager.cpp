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
#include "common/FileSystemUtil.h"
#include "common/Flags.h"
#include "logger/Logger.h"
#include "monitor/LogtailAlarm.h"
#include "common/Thread.h"

DEFINE_FLAG_INT32(adhoc_checkpoint_dump_thread_wait_interval, "microseconds", 5 * 1000 * 1000);
#if defined(__linux__)
DEFINE_FLAG_STRING(adhoc_check_point_file_dir, "", "/tmp/logtail_adhoc_checkpoint");
#elif defined(_MSC_VER)
DEFINE_FLAG_STRING(adhoc_check_point_file_dir, "", "C:\\LogtailData\\logtail_adhoc_checkpoint");
#endif

namespace logtail {

AdhocCheckpointManager::AdhocCheckpointManager() {
    new Thread([this]() { Run(); });
    LOG_INFO(sLogger, ("AdhocCheckpointManager", "Start"));
}

void AdhocCheckpointManager::Run() { 
    while (true) {
        // dump checkpoint per 5s
        usleep(INT32_FLAG(adhoc_checkpoint_dump_thread_wait_interval));

        for (auto& p : mAdhocJobCheckpointMap) {
            p.second->DumpAdhocCheckpoint(GetJobCheckpointPath(p.second->GetJobName()));
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

AdhocJobCheckpointPtr AdhocCheckpointManager::CreateAdhocJobCheckpoint(const std::string& jobName, std::vector<AdhocFileCheckpointKey> adhocFileCheckpointKeyList) {
    AdhocJobCheckpointPtr adhocJobCheckpointPtr = GetAdhocJobCheckpoint(jobName);
    if (nullptr == adhocJobCheckpointPtr) {
        adhocJobCheckpointPtr = std::make_shared<AdhocJobCheckpoint>(jobName);
        for (AdhocFileCheckpointKey key : adhocFileCheckpointKeyList) {
            adhocJobCheckpointPtr->AddAdhocFileCheckpoint(&key);
        }
        mAdhocJobCheckpointMap[jobName] = adhocJobCheckpointPtr;

        LOG_INFO(sLogger, ("Create AdhocJobCheckpoint success, job name", jobName));
        return adhocJobCheckpointPtr;
    } else {
        LOG_INFO(sLogger, ("Job checkpoint already exists, job name", jobName));
        return adhocJobCheckpointPtr;
    }
}

void AdhocCheckpointManager::DeleteAdhocJobCheckpoint(const std::string& jobName) {
    auto it = mAdhocJobCheckpointMap.find(jobName);
    if (it != mAdhocJobCheckpointMap.end()) {
        remove(GetJobCheckpointPath(jobName).c_str());
        mAdhocJobCheckpointMap.erase(jobName);
        LOG_INFO(sLogger, ("Delete AdhocJobCheckpoint success, job name", jobName));
    } else {
        LOG_WARNING(sLogger, ("Delete AdhocJobCheckpoint fail, job checkpoint doesn't exist, job name", jobName));
    }
}

void AdhocCheckpointManager::LoadAdhocCheckpoint() {
    std::string adhocCheckpointDir = GetAdhocCheckpointDirPath();
    if (CheckExistance(adhocCheckpointDir)) {
        LOG_INFO(sLogger, ("Open adhoc checkpoint dir", "success"));
        std::vector<std::string> jobList;
        if (!GetAllFiles(adhocCheckpointDir, "*", jobList)) {
            LOG_WARNING(sLogger, ("get all adhoc checkpoint files", "failed"));
            LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "Load adhoc check point files failed");
            return;
        }

        for (std::string job : jobList) {
            AdhocJobCheckpointPtr adhocJobCheckpointPtr = std::make_shared<AdhocJobCheckpoint>(job);
            if (adhocJobCheckpointPtr->LoadAdhocCheckpoint(GetJobCheckpointPath(job))) {
                mAdhocJobCheckpointMap[job] = adhocJobCheckpointPtr;
            }
        }
    } else if (!Mkdir(adhocCheckpointDir)) {
        LOG_WARNING(sLogger, ("Create adhoc checkpoint dir", "failed"));
        LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "Create adhoc check point dir failed");
    }
}

std::string AdhocCheckpointManager::GetJobCheckpointPath(const std::string& jobName) {
    std::string path = STRING_FLAG(adhoc_check_point_file_dir);
#if defined(__linux__)
    path += "/" + jobName;
#elif defined(_MSC_VER)
    path += "\\" + jobName;
#endif
    return path;
}

std::string AdhocCheckpointManager::GetAdhocCheckpointDirPath() {
    return STRING_FLAG(adhoc_check_point_file_dir);
}

} // namespace logtail