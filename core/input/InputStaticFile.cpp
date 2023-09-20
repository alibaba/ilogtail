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

#include "InputStaticFile.h"

namespace logtail {

bool InputStaticFile::Init(const Config& config) {
    mJobName = config.mConfigName;
    mAdhocFileManager = AdhocFileManager::GetInstance();
}

bool InputStaticFile::Start() {
    GetStaticFileList();
    mAdhocFileManager->AddJob(mJobName, mFilePathList);
}

bool InputStaticFile::Stop(bool isPipelineRemoving) {
    mAdhocFileManager->DeleteJob(mJobName);
}

// Init mFilePathList
void InputStaticFile::GetStaticFileList() {
    SortFileList();
}

void InputStaticFile::SortFileList() {
}

}
