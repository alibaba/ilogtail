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

void AdhocJobCheckpoint::AddFileCheckpoint(AdhocFileCheckpointPtr fileCheckpoint) {
    mRWL.lock();
    mAdhocFileCheckpointList.push_back(fileCheckpoint);
    mFileCount++;
    mRWL.unlock();
}

AdhocFileCheckpointPtr AdhocJobCheckpoint::GetFileCheckpoint(const AdhocFileCheckpointKey* fileCheckpointKey) {
    mRWL.lock_shared();

    AdhocFileCheckpointPtr fileCheckpoint = nullptr;
    if (CheckFileConsistence(fileCheckpointKey)) {
        fileCheckpoint = mAdhocFileCheckpointList[mCurrentFileIndex];
    }

    mRWL.unlock_shared();
    return fileCheckpoint;
}

bool AdhocJobCheckpoint::UpdateFileCheckpoint(const AdhocFileCheckpointKey* fileCheckpointKey, AdhocFileCheckpointPtr fileCheckpoint) {
    mRWL.lock();

    bool dumpFlag = false;
    bool indexChangeFlag = false;
    if (!CheckFileConsistence(fileCheckpointKey) || fileCheckpoint->mOffset == -1) {
        fileCheckpoint->mStatus = STATUS_LOST;
        indexChangeFlag = true;
        dumpFlag = true;
        LOG_WARNING(sLogger, ("Adhoc file lost, job name", mAdhocJobName)("file name", fileCheckpoint->mFileName)
                ("file size", fileCheckpoint->mSize)("dev", fileCheckpoint->mDevInode.dev)("inode", fileCheckpoint->mDevInode.inode));
    } else {
        if (STATUS_WAITING == fileCheckpoint->mStatus) {
            fileCheckpoint->mStatus = STATUS_LOADING;
            fileCheckpoint->mStartTime = time(NULL);
            dumpFlag = true;
            LOG_INFO(sLogger, ("Adhoc file start reading, job name", mAdhocJobName)("file name", fileCheckpoint->mFileName)
                ("file size", fileCheckpoint->mSize)("dev", fileCheckpoint->mDevInode.dev)("inode", fileCheckpoint->mDevInode.inode));
        }
        if (fileCheckpoint->mOffset == fileCheckpoint->mSize) {
            fileCheckpoint->mStatus = STATUS_FINISHED;
            indexChangeFlag = true;
            dumpFlag = true;
            LOG_INFO(sLogger, ("Adhoc file finish reading, job name", mAdhocJobName)("file name", fileCheckpoint->mFileName)
                ("file size", fileCheckpoint->mSize)("dev", fileCheckpoint->mDevInode.dev)("inode", fileCheckpoint->mDevInode.inode));
        }
    }
    fileCheckpoint->mLastUpdateTime = time(NULL);

    if (indexChangeFlag) {
        mCurrentFileIndex++;
    }

    mRWL.unlock();
    return dumpFlag;
}

bool AdhocJobCheckpoint::Load(const std::string& path) {
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
            AdhocFileCheckpointPtr fileCheckpoint = std::make_shared<AdhocFileCheckpoint>();
            fileCheckpoint->mFileName = file["file_name"].asString();
            fileCheckpoint->mStatus = GetStatusFromString(file["status"].asString());
            fileCheckpoint->mJobName = mAdhocJobName;

            switch (fileCheckpoint->mStatus) {
            case STATUS_WAITING:
                fileCheckpoint->mDevInode.dev = file["dev"].asUInt64();
                fileCheckpoint->mDevInode.inode = file["inode"].asUInt64();
                fileCheckpoint->mSize = file["size"].asInt64();
                break;
            case STATUS_LOADING:
                fileCheckpoint->mDevInode.dev = file["dev"].asUInt64();
                fileCheckpoint->mDevInode.inode = file["inode"].asUInt64();
                fileCheckpoint->mOffset = file["offset"].asInt64();
                fileCheckpoint->mSize = file["size"].asInt64();
                fileCheckpoint->mSignatureHash = file["sig_hash"].asUInt64();
                fileCheckpoint->mSignatureSize = file["sig_size"].asUInt();
                fileCheckpoint->mStartTime = file["start_time"].asInt();
                fileCheckpoint->mLastUpdateTime = file["update_time"].asInt();
                fileCheckpoint->mRealFileName = file["real_file_name"].asString();
                break;
            case STATUS_FINISHED:
                fileCheckpoint->mSize = file["size"].asInt64();
                fileCheckpoint->mStartTime = file["start_time"].asInt();
                fileCheckpoint->mLastUpdateTime = file["update_time"].asInt();
                fileCheckpoint->mRealFileName = file["real_file_name"].asString();
                break;
            case STATUS_LOST:
                fileCheckpoint->mLastUpdateTime = file["update_time"].asInt();
                break;
            }
            mAdhocFileCheckpointList.push_back(fileCheckpoint);
        }
        return true;
    } else {
        return false;
    }
}

void AdhocJobCheckpoint::Dump(const std::string& path) {
    if (!Mkdirs(ParentPath(path))) {
        LOG_ERROR(sLogger, ("open adhoc check point file dir error when dump", path));
        LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "open adhoc check point file dir failed");
        return;
    }
    mRWL.lock_shared();

    Json::Value root;
    root["job_name"] = mAdhocJobName;
    root["job_file_count"] = mFileCount;
    root["job_current_file_index"] = mCurrentFileIndex;
    Json::Value files(Json::arrayValue);

    for (AdhocFileCheckpointPtr fileCheckpoint : mAdhocFileCheckpointList) {
        Json::Value file;
        file["file_name"] = fileCheckpoint->mFileName;
        file["status"] = TransStatusToString(fileCheckpoint->mStatus);

        switch (fileCheckpoint->mStatus) {
        case STATUS_WAITING:
            file["dev"] = fileCheckpoint->mDevInode.dev;
            file["inode"] = fileCheckpoint->mDevInode.inode;
            file["size"] = fileCheckpoint->mSize;
            break;
        case STATUS_LOADING:
            file["dev"] = fileCheckpoint->mDevInode.dev;
            file["inode"] = fileCheckpoint->mDevInode.inode;
            file["offset"] = fileCheckpoint->mOffset;
            file["size"] = fileCheckpoint->mSize;
            file["sig_hash"] = fileCheckpoint->mSignatureHash;
            file["sig_size"] = fileCheckpoint->mSignatureSize;
            file["start_time"] = fileCheckpoint->mStartTime;
            file["update_time"] = fileCheckpoint->mLastUpdateTime;
            file["real_file_name"] = fileCheckpoint->mRealFileName;
            break;
        case STATUS_FINISHED:
            file["size"] = fileCheckpoint->mSize;
            file["start_time"] = fileCheckpoint->mStartTime;
            file["update_time"] = fileCheckpoint->mLastUpdateTime;
            file["real_file_name"] = fileCheckpoint->mRealFileName;
            break;
        case STATUS_LOST:
            file["update_time"] = fileCheckpoint->mLastUpdateTime;
            break;
        }

        files.append(file);
    }
    root["job_files"] = files;

    mRWL.unlock_shared();

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

bool AdhocJobCheckpoint::CheckFileConsistence(const AdhocFileCheckpointKey* fileCheckpointKey) {
    if (mCurrentFileIndex >= mFileCount) {
        LOG_WARNING(sLogger, ("Get AdhocFileCheckpoint fail, job is finished, job name", mAdhocJobName));
        return false;
    }
    AdhocFileCheckpointPtr fileCheckpoint = mAdhocFileCheckpointList[mCurrentFileIndex];
    if (fileCheckpoint->mSignatureHash == fileCheckpointKey->mSignatureHash
        && fileCheckpoint->mSignatureSize == fileCheckpointKey->mSignatureSize
        && fileCheckpoint->mDevInode == fileCheckpointKey->mDevInode) {
        return true;
    } else {
        LOG_WARNING(sLogger, 
            ("Get AdhocFileCheckpoint fail", "current FileCheckpoint in Job is not same as AdhocFileCheckpointKey")
            ("AdhocFileCheckpoint's name", fileCheckpoint->mFileName)
            ("AdhocFileCheckpoint's Inode", fileCheckpoint->mDevInode.inode)
            ("AdhocFileCheckpointKey's Inode", fileCheckpointKey->mDevInode.inode));
        return false;
    }
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