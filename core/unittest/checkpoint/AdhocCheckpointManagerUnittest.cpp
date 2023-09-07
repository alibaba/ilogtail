// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "unittest/Unittest.h"
#include "checkpoint/AdhocCheckpointManager.h"
#include "common/FileSystemUtil.h"
#include "common/Flags.h"
#include "AppConfig.h"

namespace logtail {

std::string kTestRootDir;

class AdhocCheckpointManagerUnittest {
public:
    static void SetUpTestCase() {
        kTestRootDir = (bfs::path(GetProcessExecutionDir()) / "AdhocCheckpointManagerUnittest").string();
        bfs::remove_all(kTestRootDir);
        bfs::create_directories(kTestRootDir);
        AppConfig::GetInstance()->SetLogtailSysConfDir(kTestRootDir);
    }

    static void TearDownTestCase() { bfs::remove_all(kTestRootDir); }

    void TestAdhocCheckpointManager();
};

UNIT_TEST_CASE(AdhocCheckpointManagerUnittest, TestAdhocCheckpointManager);

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManager() {
    // init
    AdhocCheckpointManager* adHocCheckpointManagerPtr = AdhocCheckpointManager::GetInstance();
    // dump checkpoint
    adHocCheckpointManagerPtr->Run();

    // new job job_a
    std::string jobA = "job_a";
    AdhocJobCheckpointPtr adHocJobCheckpointPtrA = adHocCheckpointManagerPtr->GetAdhocJobCheckpoint(jobA);
    AdhocFileCheckpointKey fileKeyA1(DevInode(11, 11), jobA);
    adHocJobCheckpointPtrA->AddAdhocFileCheckpoint(fileKeyA1);
    AdhocFileCheckpointKey fileKeyA2(DevInode(12, 12), jobA);
    adHocJobCheckpointPtrA->AddAdhocFileCheckpoint(fileKeyA2);
    AdhocFileCheckpointKey fileKeyA3(DevInode(13, 13), jobA);
    adHocJobCheckpointPtrA->AddAdhocFileCheckpoint(fileKeyA3);
    AdhocFileCheckpointKey fileKeyA4(DevInode(14, 14), jobA);
    adHocJobCheckpointPtrA->AddAdhocFileCheckpoint(fileKeyA4);

    // new job job_b
    std::string jobB = "job_b";
    AdhocJobCheckpointPtr adHocJobCheckpointPtrB = adHocCheckpointManagerPtr->GetAdhocJobCheckpoint(jobB);
    AdhocFileCheckpointKey fileKeyB1(DevInode(21, 21), jobB);
    adHocJobCheckpointPtrB->AddAdhocFileCheckpoint(fileKeyB1);
    AdhocFileCheckpointKey fileKeyB2(DevInode(22, 22), jobB);
    adHocJobCheckpointPtrB->AddAdhocFileCheckpoint(fileKeyB2);
    AdhocFileCheckpointKey fileKeyB3(DevInode(23, 23), jobB);
    adHocJobCheckpointPtrB->AddAdhocFileCheckpoint(fileKeyB3);

    // if delete job_a
    adHocJobCheckpointPtrA->Delete();

    // update checkpoint
    AdhocFileCheckpointPtr fileB1 = adHocJobCheckpointPtrB->GetAdhocFileCheckpoint(fileKeyB1);
    fileB1->mOffset = 666666;
    adHocJobCheckpointPtrB->UpdateAdhocFileCheckpoint(fileB1);
    AdhocFileCheckpointPtr fileB2 = adHocJobCheckpointPtrB->GetAdhocFileCheckpoint(fileKeyB2);
    fileB2->mOffset = 88888888;
    adHocJobCheckpointPtrB->UpdateAdhocFileCheckpoint(fileB2);
}


} // namespace logtail

UNIT_TEST_MAIN
