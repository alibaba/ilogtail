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

#include <json/json.h>
#include "AdhocJobCheckpoint.h"
#include "common/FileSystemUtil.h"
#include "logger/Logger.h"
#include "monitor/LogtailAlarm.h"

namespace logtail {

AdhocJobCheckpoint::AdhocJobCheckpoint(const std::string& jobName) {
    mAdhocJobName = jobName;
    mFileCount = 0;
    mCurrentFileIndex = 0;
}

AdhocJobCheckpoint::~AdhocJobCheckpoint() {
}

void AdhocJobCheckpoint::AddAdhocFileCheckpoint(const AdhocFileCheckpointKey* adhocFileCheckpointKey) {
    if ("" != adhocFileCheckpointKey->mFileName || 0 != adhocFileCheckpointKey->mFileSize
        || DevInode() != adhocFileCheckpointKey->mDevInode || "" != mAdhocJobName) {
            LOG_WARNING(sLogger, ("Add AdhocFileCheckpoint fail, file checkpoint info", adhocFileCheckpointKey));
            return;
        }
    AdhocFileCheckpointPtr ptr = std::make_shared<AdhocFileCheckpoint>(
        adhocFileCheckpointKey->mFileName,
        adhocFileCheckpointKey->mFileSize,
        0,
        0,
        0,
        adhocFileCheckpointKey->mDevInode,
        STATUS_WAITING,
        mAdhocJobName);
    mAdhocFileCheckpointList.push_back(ptr);
    mFileCount++;
}

AdhocFileCheckpointPtr AdhocJobCheckpoint::GetAdhocFileCheckpoint(const AdhocFileCheckpointKey* adhocFileCheckpointKey) {
    if (!CheckFileInList(adhocFileCheckpointKey)) {
        return nullptr;
    }

    return mAdhocFileCheckpointList[mCurrentFileIndex];
}

bool AdhocJobCheckpoint::UpdateAdhocFileCheckpoint(const AdhocFileCheckpointKey* adhocFileCheckpointKey, AdhocFileCheckpointPtr ptr) {
    if (!CheckFileInList(adhocFileCheckpointKey)) {
        return -1;
    }

    bool dumpFlag = false;
    bool indexChangeFlag = false;
    if (ptr->mOffset == -1) {
        ptr->mStatus = STATUS_LOST;
        indexChangeFlag = true;
        dumpFlag = true;
    } else {
        if (STATUS_WAITING == ptr->mStatus) {
            ptr->mStatus = STATUS_LOADING;
            dumpFlag = true;
        }
        if (ptr->mOffset == ptr->mSize) {
            ptr->mStatus = STATUS_FINISHED;
            indexChangeFlag = true;
            dumpFlag = true;
        }
    }

    if (indexChangeFlag) {
        mCurrentFileIndex++;
    }

    return dumpFlag;
}

bool AdhocJobCheckpoint::LoadAdhocCheckpoint(const std::string& path) {
    if (CheckExistance(path)) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            LOG_ERROR(sLogger, ("open adhoc check point file error when load", path));
            LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "open check point file failed");
            return false;
        }

        Json::Value root;
        ifs >> root;
        ifs.close();

        mAdhocJobName = root["job_name"].asString();
        mFileCount = root["job_file_count"].asInt();
        mCurrentFileIndex = root["job_current_file_index"].asInt();

        Json::Value files = root["job_files"];

        for (Json::Value file : files) {
            AdhocFileCheckpointPtr ptr = std::make_shared<AdhocFileCheckpoint>();
            ptr->mFileName = file["file_name"].asString();
            ptr->mStatus = GetStatusFromString(file["status"].asString());
            ptr->mJobName = mAdhocJobName;

            switch (ptr->mStatus) {
            case STATUS_WAITING:
                ptr->mDevInode.dev = file["dev"].asUInt64();
                ptr->mDevInode.inode = file["inode"].asUInt64();
                ptr->mSize = file["size"].asInt64();
                break;
            case STATUS_LOADING:
                ptr->mDevInode.dev = file["dev"].asUInt64();
                ptr->mDevInode.inode = file["inode"].asUInt64();
                ptr->mOffset = file["offset"].asInt64();
                ptr->mSize = file["size"].asInt64();
                ptr->mSignatureHash = file["sig_hash"].asUInt64();
                ptr->mSignatureSize = file["sig_size"].asUInt();
                ptr->mStartTime = file["start_time"].asInt();
                ptr->mLastUpdateTime = file["update_time"].asInt();
                break;
            case STATUS_FINISHED:
                ptr->mSize = file["size"].asInt64();
                ptr->mStartTime = file["start_time"].asInt();
                ptr->mLastUpdateTime = file["update_time"].asInt();
                break;
            case STATUS_LOST:
                ptr->mLastUpdateTime = file["update_time"].asInt();
                break;
            }
            mAdhocFileCheckpointList.push_back(ptr);
        }
        return true;
    } else {
        return false;
    }
}

void AdhocJobCheckpoint::DumpAdhocCheckpoint(const std::string& path) {
    if (!Mkdirs(ParentPath(path))) {
        LOG_ERROR(sLogger, ("open adhoc check point file dir error when dump", path));
        LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "open adhoc check point file dir failed");
        return;
    }
    std::lock_guard<std::mutex> lock(mMutex);

    Json::Value root;
    root["job_name"] = mAdhocJobName;
    root["job_file_count"] = mFileCount;
    root["job_current_file_index"] = mCurrentFileIndex;
    Json::Value files(Json::arrayValue);

    for (AdhocFileCheckpointPtr ptr : mAdhocFileCheckpointList) {
        std::lock_guard<std::mutex> lock(ptr->mMutex);
        ptr->mLastUpdateTime = time(NULL);

        Json::Value file;
        file["file_name"] = ptr->mFileName;
        file["status"] = TransStatusToString(ptr->mStatus);

        switch (ptr->mStatus) {
        case STATUS_WAITING:
            file["dev"] = ptr->mDevInode.dev;
            file["inode"] = ptr->mDevInode.inode;
            file["size"] = ptr->mSize;
            break;
        case STATUS_LOADING:
            file["dev"] = ptr->mDevInode.dev;
            file["inode"] = ptr->mDevInode.inode;
            file["offset"] = ptr->mOffset;
            file["size"] = ptr->mSize;
            file["sig_hash"] = ptr->mSignatureHash;
            file["sig_size"] = ptr->mSignatureSize;
            file["start_time"] = ptr->mStartTime;
            file["update_time"] = ptr->mLastUpdateTime;
            break;
        case STATUS_FINISHED:
            file["size"] = ptr->mSize;
            file["start_time"] = ptr->mStartTime;
            file["update_time"] = ptr->mLastUpdateTime;
            break;
        case STATUS_LOST:
            file["update_time"] = ptr->mLastUpdateTime;
            break;
        }

        files.append(file);
    }

    root["job_files"] = files;

    Json::StreamWriterBuilder writerBuilder;
    std::string jsonString = Json::writeString(writerBuilder, root);
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        LOG_ERROR(sLogger, ("open adhoc check point file error", path));
        LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "open adhoc check point file failed");
        return;
    }
    ofs << jsonString;
    ofs.close();
}

bool AdhocJobCheckpoint::CheckFileInList(const AdhocFileCheckpointKey* adhocFileCheckpointKey) {
    if (mCurrentFileIndex >= mFileCount) {
        return false;
    }
    AdhocFileCheckpointPtr ptr = mAdhocFileCheckpointList[mCurrentFileIndex];
    if (ptr->mFileName == adhocFileCheckpointKey->mFileName &&
        ptr->mDevInode == adhocFileCheckpointKey->mDevInode) {
            return true;
        }
    return false;
}

int32_t AdhocJobCheckpoint::GetCurrentFileIndex() {
    return mCurrentFileIndex;
}

std::string AdhocJobCheckpoint::GetJobName() {
    return mAdhocJobName;
}

std::vector<std::string> AdhocJobCheckpoint::GetFileList() {
    std::vector<std::string> result;
    for (AdhocFileCheckpointPtr filePtr : mAdhocFileCheckpointList) {
        result.push_back(filePtr->mFileName);
    }
    return result;
}

} // namespace logtail