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
#include <random>
#include "unittest/Unittest.h"
#include "checkpoint/AdhocCheckpointManager.h"
#include "common/FileSystemUtil.h"
#include "AppConfig.h"

namespace logtail {

std::string kTestRootDir;
AdhocCheckpointManager* mAdhocCheckpointManager;

class AdhocCheckpointManagerUnittest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        kTestRootDir = (bfs::path(GetProcessExecutionDir()) / "AdhocCheckpointManagerUnittest").string();
        bfs::remove_all(kTestRootDir);
        bfs::create_directories(kTestRootDir);
        AppConfig::GetInstance()->SetLogtailSysConfDir(kTestRootDir);
        mAdhocCheckpointManager = AdhocCheckpointManager::GetInstance();
    }

    static void TearDownTestCase() { bfs::remove_all(kTestRootDir); }

    /*
    create 2 job: 
        test_job_A
            test_file_A_1, test_file_A_2, test_file_A_3
        test_job_B
            test_file_B_1, test_file_B_2
    */ 
    void TestAdhocCheckpointManagerStart();
    /*
    update status of files:
        test_job_A
            test_file_A_1 -> finished
            test_file_A_2 -> loading
            test_file_A_3 -> waiting
        test_job_B
            test_file_B_1 -> lost
            test_file_B_2 -> finished
    */
    void TestAdhocCheckpointManagerRun();
    /*
    check checkpoint files:
        /tmp/logtail_adhoc_checkpoint/test_job_A
        /tmp/logtail_adhoc_checkpoint/test_job_B
    */
    void TestAdhocCheckpointManagerDump();
    /*
    delete checkpoint files:
        /tmp/logtail_adhoc_checkpoint/test_job_A
        /tmp/logtail_adhoc_checkpoint/test_job_B
    */
    void TestAdhocCheckpointManagerStop();

private:
    void AddJob(const std::string& jobName, const std::vector<std::string> jobFiles);
    void MockFileLog(const std::string& jobName, const std::string& fileName);
    AdhocFileCheckpointKey* GetAdhocFileCheckpointKey(const std::string& jobName, const std::string& fileName);

    std::string jobName1 = "test_job_A";
    std::vector<std::string> jobFiles1 = {"test_file_A_1", "test_file_A_2", "test_file_A_3"};
    std::string jobName2 = "test_job_B";
    std::vector<std::string> jobFiles2 = {"test_file_B_1", "test_file_B_2"};
};

UNIT_TEST_CASE(AdhocCheckpointManagerUnittest, TestAdhocCheckpointManagerStart);
UNIT_TEST_CASE(AdhocCheckpointManagerUnittest, TestAdhocCheckpointManagerRun);
UNIT_TEST_CASE(AdhocCheckpointManagerUnittest, TestAdhocCheckpointManagerDump);
UNIT_TEST_CASE(AdhocCheckpointManagerUnittest, TestAdhocCheckpointManagerStop);

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManagerStart() {
    // logtail start, AdhocCheckpointManager start
    mAdhocCheckpointManager->LoadAdhocCheckpoint();

    // job start
    // job 1
    AddJob(jobName1, jobFiles1);
    AdhocJobCheckpointPtr jobCheckpoint1 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName1);
    EXPECT_EQ(jobCheckpoint1->GetJobName(), jobName1);
    // job 2
    AddJob(jobName2, jobFiles2);
    AdhocJobCheckpointPtr jobCheckpoint2 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName2);
    EXPECT_EQ(jobCheckpoint2->GetJobName(), jobName2);
    // job 3
    std::string jobName3 = jobName1; 
    std::vector<std::string> jobFiles3 = {"test_file_C_1"};
    AddJob(jobName3, jobFiles3);
    AdhocJobCheckpointPtr jobCheckpoint3
        = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName3); // return jobCheckpoint1
    EXPECT_EQ(jobCheckpoint3->GetJobName(), jobName3);
    std::vector<std::string> jobFilesTmp = jobCheckpoint3->GetFileList();
    EXPECT_EQ(jobFilesTmp.size(), jobFiles1.size());
    for (size_t i = 0; i < jobFilesTmp.size(); i++) {
        EXPECT_EQ(jobFilesTmp[i].find(jobFiles1[i]) != std::string::npos, true);
    }
}

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManagerRun() {
    // Get job checkpoint
    AdhocJobCheckpointPtr jobCheckpoint1 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName1);
    AdhocJobCheckpointPtr jobCheckpoint2 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName2);

    AdhocFileCheckpointPtr fileCheckpointA1 = jobCheckpoint1->GetAdhocFileCheckpoint(GetAdhocFileCheckpointKey(jobName1, jobFiles1[0]));
    EXPECT_EQ(nullptr != fileCheckpointA1, true);
    // Get file checkpoint

    // Update job checkpoint

    // Update file checkpoint 
}

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManagerDump() {
    // check dump when file status change

    // check dump when auto dump per 5s
    usleep(10 * 1000 * 1000);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), true);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2)), true);
}

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManagerStop() {
    // test delete
    mAdhocCheckpointManager->DeleteAdhocJobCheckpoint(jobName1);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), false);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2)), true);

    mAdhocCheckpointManager->DeleteAdhocJobCheckpoint(jobName2);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), false);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2)), false);
}


void AdhocCheckpointManagerUnittest::AddJob(const std::string& jobName, const std::vector<std::string> jobFiles) {
    std::vector<AdhocFileCheckpointKey> keys;
    for (std::string fileName : jobFiles) {        
        MockFileLog(jobName, fileName);
        keys.push_back(*GetAdhocFileCheckpointKey(jobName, fileName));
    }
    AdhocJobCheckpointPtr jobCheckpoint
        = AdhocCheckpointManager::GetInstance()->CreateAdhocJobCheckpoint(jobName, keys);
    EXPECT_EQ(nullptr != jobCheckpoint, true);
}

void AdhocCheckpointManagerUnittest::MockFileLog(const std::string& jobName, const std::string& fileName) {
    std::string jobDataPath = GetProcessExecutionDir() + jobName;
    std::string filePath = PathJoin(jobDataPath, fileName);

    Mkdir(jobDataPath);
    std::ofstream file(filePath); 

    if (!file) {
        std::cout << "Create file failed" << filePath << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 100);

    int randomLogCount = dist(gen);
    for (int i = 0; i < randomLogCount; ++i) {
        int randomNumber = dist(gen);
        std::time_t now = std::time(nullptr);
        std::string log = std::to_string(randomNumber) + " - " + std::ctime(&now);
        file << log;
    }

    file.close();  // 关闭文件
}

AdhocFileCheckpointKey* AdhocCheckpointManagerUnittest::GetAdhocFileCheckpointKey(const std::string& jobName, const std::string& fileName) {
    std::string jobDataPath = GetProcessExecutionDir() + jobName;
    std::string filePath = PathJoin(jobDataPath, fileName);

    struct stat statbuf;
    stat(filePath.c_str(), &statbuf);
    int64_t fileSize = statbuf.st_size;
    AdhocFileCheckpointKey* key = new AdhocFileCheckpointKey(GetFileDevInode(filePath), filePath, fileSize);
    return key;
}

} // namespace logtail

UNIT_TEST_MAIN