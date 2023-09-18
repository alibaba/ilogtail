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
    wait 10ms
    update offset of test_file_A_2(should not dump)
    wait 5s(auto dump)
    check checkpoint files:
        /tmp/logtail_adhoc_checkpoint/test_job_A
        /tmp/logtail_adhoc_checkpoint/test_job_B
    */
    void TestAdhocCheckpointManagerDump();
    /*
    delete checkpoint files:
        /tmp/logtail_adhoc_checkpoint/test_job_A
        /tmp/logtail_adhoc_checkpoint/test_job_B
    check checkpoint files(should not exist):
        /tmp/logtail_adhoc_checkpoint/test_job_A
        /tmp/logtail_adhoc_checkpoint/test_job_B
    */
    void TestAdhocCheckpointManagerStop();

private:
    void AddJob(const std::string& jobName, const std::vector<std::string> jobFiles);
    void MockFileLog(const std::string& jobName, const std::string& fileName);
    AdhocFileCheckpointKey* GetAdhocFileCheckpointKey(const std::string& jobName, const std::string& fileName);
    long GetFileUpdateTime(const std::string& filePath);

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
    EXPECT_EQ(nullptr != jobCheckpoint1, true);
    EXPECT_EQ(nullptr != jobCheckpoint2, true);

    // Get file checkpoint
    AdhocFileCheckpointKey* fileChakepointKeyA1 = GetAdhocFileCheckpointKey(jobName1, jobFiles1[0]);
    AdhocFileCheckpointPtr fileCheckpointA1 = jobCheckpoint1->GetAdhocFileCheckpoint(fileChakepointKeyA1);
    EXPECT_EQ(nullptr != fileCheckpointA1, true);
    AdhocFileCheckpointKey* fileChakepointKeyA2 = GetAdhocFileCheckpointKey(jobName1, jobFiles1[1]);
    AdhocFileCheckpointPtr fileCheckpointA2 = jobCheckpoint1->GetAdhocFileCheckpoint(fileChakepointKeyA2);
    EXPECT_EQ(nullptr != fileCheckpointA2, false);

    // Read test_file_A_1
    fileCheckpointA1->mOffset = fileChakepointKeyA1->mFileSize;
    if (jobCheckpoint1->UpdateAdhocFileCheckpoint(fileChakepointKeyA1, fileCheckpointA1)) {
        jobCheckpoint1->DumpAdhocCheckpoint(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1));
    }
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), true);
    EXPECT_EQ(fileCheckpointA1->mStatus == STATUS_FINISHED, true);

    // Read test_file_A_2 
    fileCheckpointA2 = jobCheckpoint1->GetAdhocFileCheckpoint(fileChakepointKeyA2);
    EXPECT_EQ(nullptr != fileCheckpointA2, true);
    fileCheckpointA2->mOffset = fileChakepointKeyA2->mFileSize / 3;
    if (jobCheckpoint1->UpdateAdhocFileCheckpoint(fileChakepointKeyA2, fileCheckpointA2)) {
        jobCheckpoint1->DumpAdhocCheckpoint(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1));
    }
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), true);
    EXPECT_EQ(fileCheckpointA2->mStatus == STATUS_LOADING, true);

    // Read test_file_B_1 
    AdhocFileCheckpointKey* fileChakepointKeyB1 = GetAdhocFileCheckpointKey(jobName2, jobFiles2[0]);
    AdhocFileCheckpointPtr fileCheckpointB1 = jobCheckpoint2->GetAdhocFileCheckpoint(fileChakepointKeyB1);
    EXPECT_EQ(nullptr != fileCheckpointB1, true);
    fileCheckpointB1->mOffset = -1;
    if (jobCheckpoint2->UpdateAdhocFileCheckpoint(fileChakepointKeyB1, fileCheckpointB1)) {
        jobCheckpoint2->DumpAdhocCheckpoint(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2));
    }
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2)), true);
    EXPECT_EQ(fileCheckpointB1->mStatus == STATUS_LOST, true);

    // Read test_file_B_2 
    AdhocFileCheckpointKey* fileChakepointKeyB2 = GetAdhocFileCheckpointKey(jobName2, jobFiles2[1]);
    AdhocFileCheckpointPtr fileCheckpointB2 = jobCheckpoint2->GetAdhocFileCheckpoint(fileChakepointKeyB2);
    EXPECT_EQ(nullptr != fileCheckpointB2, true);
    fileCheckpointB2->mOffset = fileChakepointKeyB2->mFileSize;
    if (jobCheckpoint2->UpdateAdhocFileCheckpoint(fileChakepointKeyB2, fileCheckpointB2)) {
        jobCheckpoint2->DumpAdhocCheckpoint(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2));
    }
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2)), true);
    EXPECT_EQ(fileCheckpointB2->mStatus == STATUS_FINISHED, true);
}

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManagerDump() {
    // check dump when file status change
    AdhocJobCheckpointPtr jobCheckpoint1 = mAdhocCheckpointManager->GetAdhocJobCheckpoint(jobName1);
    AdhocFileCheckpointKey* fileChakepointKeyA2 = GetAdhocFileCheckpointKey(jobName1, jobFiles1[1]);
    AdhocFileCheckpointPtr fileCheckpointA2 = jobCheckpoint1->GetAdhocFileCheckpoint(fileChakepointKeyA2);
    EXPECT_EQ(nullptr != fileCheckpointA2, true);
    fileCheckpointA2->mOffset = fileChakepointKeyA2->mFileSize / 3 * 2;

    usleep(1 * 1000 * 1000);
    EXPECT_EQ(jobCheckpoint1->UpdateAdhocFileCheckpoint(fileChakepointKeyA2, fileCheckpointA2), false);
    EXPECT_EQ(fileCheckpointA2->mStatus == STATUS_LOADING, true);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), true);
    long time1 = GetFileUpdateTime(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1));

    // check dump when auto dump per 5s
    usleep(5 * 1000 * 1000);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), true);
    long time2 = GetFileUpdateTime(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1));
    EXPECT_EQ(time1 != time2, true);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2)), true);
}

void AdhocCheckpointManagerUnittest::TestAdhocCheckpointManagerStop() {
    // test delete
    mAdhocCheckpointManager->DeleteAdhocJobCheckpoint(jobName2);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName1)), true);
    EXPECT_EQ(CheckExistance(mAdhocCheckpointManager->GetJobCheckpointPath(jobName2)), false);

    usleep(5 * 1000 * 1000);
    mAdhocCheckpointManager->DeleteAdhocJobCheckpoint(jobName1);
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
    std::uniform_int_distribution<int> dist(10, 100);

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

long AdhocCheckpointManagerUnittest::GetFileUpdateTime(const std::string& filePath) {
    struct stat statbuf;
    stat(filePath.c_str(), &statbuf);
    return statbuf.st_mtim.tv_nsec;
}

} // namespace logtail

UNIT_TEST_MAIN