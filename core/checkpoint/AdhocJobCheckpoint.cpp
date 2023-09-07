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
    mFilePos = 0;
    mDeleteFlag = false;
}

AdhocJobCheckpoint::~AdhocJobCheckpoint() {
}

void AdhocJobCheckpoint::AddAdhocFileCheckpoint(AdhocFileCheckpointKey adHocFileCheckpointKey) {
}

AdhocFileCheckpointPtr AdhocJobCheckpoint::GetAdhocFileCheckpoint(AdhocFileCheckpointKey adHocFileCheckpointKey) {
}

int32_t AdhocJobCheckpoint::UpdateAdhocFileCheckpoint(AdhocFileCheckpointKey adHocFileCheckpointKey, AdhocFileCheckpointPtr adHocFileCheckpointPtr) {
    return mFilePos;
}

void AdhocJobCheckpoint::DumpAdhocCheckpoint() {
}

void AdhocJobCheckpoint::Delete() {
}

} // namespace logtail