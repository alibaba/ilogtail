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
    mDeleteFlag = false;
#if defined(__linux__)
    mStorePath = STRING_FLAG(adhoc_check_point_file_dir) + "/" + jobName;
#elif defined(_MSC_VER)
    mStorePath = STRING_FLAG(adhoc_check_point_file_dir) + "\\" + jobName;
#endif
}

AdhocJobCheckpoint::~AdhocJobCheckpoint() {
}

void AdhocJobCheckpoint::AddAdhocFileCheckpoint(AdhocFileCheckpointKey adhocFileCheckpointKey) {
    AdhocFileCheckpointPtr ptr = std::make_shared<AdhocFileCheckpoint>(
        adhocFileCheckpointKey.mFileName,
        adhocFileCheckpointKey.mFileSize,
        0,
        0,
        0,
        adhocFileCheckpointKey.mDevInode,
        mAdhocJobName,
        "",
        0,
        STATUS_WAITING);
    mAdhocFileCheckpointList.push_back(ptr);
    mFileCount++;
}

AdhocFileCheckpointPtr AdhocJobCheckpoint::GetAdhocFileCheckpoint(AdhocFileCheckpointKey adhocFileCheckpointKey) {
    if (!CheckFileInList(adhocFileCheckpointKey)) {
        return nullptr;
    }

    return mAdhocFileCheckpointList[mCurrentFileIndex];
}

bool AdhocJobCheckpoint::UpdateAdhocFileCheckpoint(AdhocFileCheckpointKey adhocFileCheckpointKey, AdhocFileCheckpointPtr ptr) {
    if (!CheckFileInList(adhocFileCheckpointKey)) {
        return -1;
    }

    bool dumpFlag = false;
    if (ptr->mOffset == -1) {
        ptr->mStatus = STATUS_LOST;
        mCurrentFileIndex++;
        dumpFlag = true;
    } else {
        if (STATUS_WAITING == ptr->mStatus) {
            ptr->mStatus = STATUS_LOADING;
            dumpFlag = true;
        }
        if (ptr->mOffset == ptr->mSize) {
            ptr->mStatus = STATUS_FINISHED;
            mCurrentFileIndex++;
            dumpFlag = true;
        }
    }

    return dumpFlag;
}

void AdhocJobCheckpoint::LoadAdhocCheckpoint() {
    if (CheckExistance(mStorePath)) {
        std::ifstream ifs("json_file.json");
        if (!ifs.is_open()) {
            LOG_ERROR(sLogger, ("open adhoc check point file error when load", mStorePath));
            LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "open check point file failed");
            return;
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
                ptr->mFileOpenFlag = file["file_open"].asBool();
                ptr->mOffset = file["offset"].asInt64();
                ptr->mSize = file["size"].asInt64();
                ptr->mRealFileName = file["real_file_name"].asString();
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
    }
}

void AdhocJobCheckpoint::DumpAdhocCheckpoint() {
    if (!Mkdirs(ParentPath(mStorePath))) {
        LOG_ERROR(sLogger, ("open adhoc check point file dir error when dump", mStorePath));
        LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "open adhoc check point file dir failed");
        return;
    }

    Json::Value root;
    root["job_name"] = mAdhocJobName;
    root["job_file_count"] = mFileCount;
    root["job_current_file_index"] = mCurrentFileIndex;
    Json::Value files(Json::arrayValue);

    for (AdhocFileCheckpointPtr ptr : mAdhocFileCheckpointList) {
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
            file["file_open"] = ptr->mFileOpenFlag;
            file["offset"] = ptr->mOffset;
            file["size"] = ptr->mSize;
            file["real_file_name"] = ptr->mRealFileName;
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
    std::ofstream ofs(mStorePath);
    if (!ofs.is_open()) {
        LOG_ERROR(sLogger, ("open adhoc check point file error", mStorePath));
        LogtailAlarm::GetInstance()->SendAlarm(CHECKPOINT_ALARM, "open adhoc check point file failed");
        return;
    }
    ofs << jsonString;
    ofs.close();
}

void AdhocJobCheckpoint::Delete() {
    for (AdhocFileCheckpointPtr ptr : mAdhocFileCheckpointList) {
        ptr.reset();
    }
    remove(mStorePath.c_str());
}

bool AdhocJobCheckpoint::CheckFileInList(AdhocFileCheckpointKey adhocFileCheckpointKey) {
    if (mCurrentFileIndex >= mFileCount) {
        return false;
    }
    AdhocFileCheckpointPtr ptr = mAdhocFileCheckpointList[mCurrentFileIndex];
    if (ptr->mFileName == adhocFileCheckpointKey.mFileName &&
        ptr->mDevInode == adhocFileCheckpointKey.mDevInode) {
            return true;
        }
    return false;
}

} // namespace logtail