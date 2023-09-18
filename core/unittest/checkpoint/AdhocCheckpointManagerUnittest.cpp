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

#include <sys/stat.h>
#include "unittest/Unittest.h"
#include "checkpoint/AdhocCheckpointManager.h"
#include "common/FileSystemUtil.h"
#include "AppConfig.h"

namespace logtail {

std::string kTestRootDir;

class AdhocCheckpointManagerUnittest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        kTestRootDir = (bfs::path(GetProcessExecutionDir()) / "AdhocCheckpointManagerUnittest").string();
        bfs::remove_all(kTestRootDir);
        bfs::create_directories(kTestRootDir);
        AppConfig::GetInstance()->SetLogtailSysConfDir(kTestRootDir);
    }

    static void TearDownTestCase() { bfs::remove_all(kTestRootDir); }

    void TestAdhocCheckpointManager();
private:
    void AddJob(const std::string& jobName, const std::vector<std::string> jobFiles);
};

UNIT_TEST_CASE(AdhocCheckpointManagerUnittest, TestAdhocCheckpointManager);

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManager() {
    AdhocCheckpointManager* mAdhocCheckpointManager = AdhocCheckpointManager::GetInstance();
    // test start, add job
    {
        // logtail start, AdhocCheckpointManager start
        mAdhocCheckpointManager->LoadAdhocCheckpoint();

        // job start
        // job 1
        std::string jobName1 = "test_job_A";
        std::vector<std::string> jobFiles1 = {"test_file_A_1"};
        AddJob(jobName1, jobFiles1);
        AdhocJobCheckpointPtr jobCheckpoint1 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName1);
        EXPECT_EQ(jobCheckpoint1->GetJobName(), jobName1);
        // job 2
        std::string jobName2 = "test_job_B";
        std::vector<std::string> jobFiles2 = {"test_file_B_1"};
        AddJob(jobName2, jobFiles2);
        AdhocJobCheckpointPtr jobCheckpoint2 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName2);
        EXPECT_EQ(jobCheckpoint2->GetJobName(), jobName2);
        // job 3
        std::string jobName3 = "test_job_A"; // = jobName1
        std::vector<std::string> jobFiles3 = {"test_file_C_1"};
        AddJob(jobName3, jobFiles3);
        AdhocJobCheckpointPtr jobCheckpoint3 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName3); // return jobCheckpoint1
        EXPECT_EQ(jobCheckpoint3->GetJobName(), jobName3);
        std::vector<std::string> jobFilesTmp = jobCheckpoint3->GetFileList();
        EXPECT_TRUE(jobFilesTmp.size() == jobFiles1.size());
        for (int i = 0; i < jobFilesTmp.size(); i++) {
            EXPECT_TRUE(jobFilesTmp[i].substr(jobFilesTmp[i].size()-jobFiles1[i].size()) == jobFiles1[i]);
        }
    }
    // test run
    {

    }
    // test stop 
}

void AdhocCheckpointManagerUnittest::AddJob(const std::string& jobName, const std::vector<std::string> jobFiles) {
    std::string jobDataPath = GetProcessExecutionDir() + jobName;

    std::vector<AdhocFileCheckpointKey> keys;
    for (std::string fileName : jobFiles) {
        std::string filePath = PathJoin(jobDataPath, fileName);
        struct stat statbuf;
        stat(filePath.c_str(), &statbuf);
        int64_t fileSize = statbuf.st_size;
        AdhocFileCheckpointKey key(GetFileDevInode(filePath), jobName, fileSize);
        keys.push_back(key);
    }
    AdhocJobCheckpointPtr jobCheckpoint = AdhocCheckpointManager::GetInstance()->CreateAdhocJobCheckpoint(jobName, keys);
    EXPECT_FALSE(nullptr == jobCheckpoint);
}

} // namespace logtail

UNIT_TEST_MAIN
