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

#include "AdhocJobCheckpoint.h"

namespace logtail {

AdhocJobCheckpoint::AdhocJobCheckpoint(const std::string& jobName) {
    mAdhocJobName = jobName;
    mFileCount = 0;
    mCurrentFileIndex = 0;
    mDeleteFlag = false;
}

AdhocJobCheckpoint::~AdhocJobCheckpoint() {
}

void AdhocJobCheckpoint::AddAdhocFileCheckpoint(AdhocFileCheckpointKey adhocFileCheckpointKey) {
    AdhocFileCheckpointPtr ptr = std::make_shared<AdhocFileCheckpoint>(
        adhocFileCheckpointKey.mFileName,
        0,
        adhocFileCheckpointKey.mDevInode,
        mAdhocJobName,
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

void AdhocJobCheckpoint::DumpAdhocCheckpoint() {
}

void AdhocJobCheckpoint::Delete() {
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