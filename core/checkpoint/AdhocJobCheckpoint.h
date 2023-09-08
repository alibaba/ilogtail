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
#include <vector>
#include "AdhocFileCheckpoint.h"
#include "common/Flags.h"

#if defined(__linux__)
DEFINE_FLAG_STRING(adhoc_check_point_file_dir, "", "/tmp/logtail_adhoc_checkpoint");
#elif defined(_MSC_VER)
DEFINE_FLAG_STRING(adhoc_check_point_file_dir, "", "C:\\LogtailData\\logtail_adhoc_checkpoint");
#endif

namespace logtail {

class AdhocJobCheckpoint {
private:
    bool CheckFileInList(AdhocFileCheckpointKey adhocFileCheckpointKey);
    std::vector<AdhocFileCheckpointPtr> mAdhocFileCheckpointList;
    int32_t mFileCount;
    int32_t mCurrentFileIndex;
    std::string mAdhocJobName; 
    std::string mStorePath; 
    bool mDeleteFlag;

public:
    AdhocJobCheckpoint(const std::string& jobName);
    ~AdhocJobCheckpoint();

    AdhocFileCheckpointPtr GetAdhocFileCheckpoint(AdhocFileCheckpointKey adhocFileCheckpointKey);
    bool UpdateAdhocFileCheckpoint(AdhocFileCheckpointKey adhocFileCheckpointKey, AdhocFileCheckpointPtr adhocFileCheckpointPtr);
    void LoadAdhocCheckpoint();
    void DumpAdhocCheckpoint();
    void Delete();

    void AddAdhocFileCheckpoint(AdhocFileCheckpointKey adhocFileCheckpointKey);
    int32_t GetCurrentFileIndex() { return mCurrentFileIndex; }
};

typedef std::shared_ptr<AdhocJobCheckpoint> AdhocJobCheckpointPtr;

} // namespace logtail
