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
#include <memory>
#include <string>
#include "common/DevInode.h"

namespace logtail {

enum FileReadStatus {
    STATUS_WAITING,
    STATUS_LOADING,
    STATUS_FINISHED,
    STATUS_LOST,
};

class AdhocFileCheckpoint {
private:
    /* data */
public:
    AdhocFileCheckpoint() {}

    AdhocFileCheckpoint(const std::string& filename,
                        int64_t offset,
                        uint32_t signatureSize,
                        uint64_t signatureHash,
                        DevInode devInode,
                        const std::string& jobName = std::string(),
                        const std::string& realFileName = std::string(),
                        int32_t fileOpenFlag = 0,
                        FileReadStatus status)
        : mFileName(filename),
          mRealFileName(realFileName),
          mOffset(offset),
          mSignatureSize(signatureSize),
          mSignatureHash(signatureHash),
          mStartTime(0),
          mLastUpdateTime(0),
          mDevInode(devInode),
          mFileOpenFlag(fileOpenFlag),
          mStatus(status),
          mJobName(jobName) {}
    std::string mFileName;
    ~AdhocFileCheckpoint();

    std::string mRealFileName;
    int64_t mOffset;
    int64_t mSize;
    uint32_t mSignatureSize;
    uint64_t mSignatureHash;
    int32_t mStartTime;
    int32_t mLastUpdateTime;
    DevInode mDevInode;
    int32_t mFileOpenFlag;
    FileReadStatus mStatus;
    std::string mJobName;
};

struct AdhocFileCheckpointKey {
    AdhocFileCheckpointKey() {}
    AdhocFileCheckpointKey(const DevInode& devInode, const std::string& fileName)
        : mDevInode(devInode), mFileName(fileName) {}
    DevInode mDevInode;
    std::string mFileName;
};

typedef std::shared_ptr<AdhocFileCheckpoint> AdhocFileCheckpointPtr;

} // namespace logtail