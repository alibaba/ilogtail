#include <gtest/gtest.h>
#include "common/ContainerInfoProvider.h"
#include "common/FileUtils.h"
#include "common/UnitTestEnv.h"
#include "common/Chrono.h"

#include <memory> // unique_ptr

using namespace common;
using namespace std;

class CommonContainerInfoProviderTest:public testing::Test{
protected:
   void SetUp() override {

  }
void TearDown() override{

  }
};
TEST_F(CommonContainerInfoProviderTest,ParseDockerInspectInfos)
{
    unique_ptr<ContainerInfoProvider> pContainerInfoProviderPtr(new ContainerInfoProvider());
    pContainerInfoProviderPtr->UpdateConfig("/var/run/docker.sock1","http://localhost/containers/json1",70000000);
    EXPECT_EQ(pContainerInfoProviderPtr->mUnixSocketPath,"/var/run/docker.sock1");
    EXPECT_EQ(pContainerInfoProviderPtr->mUrl,"http://localhost/containers/json1");
    EXPECT_EQ(pContainerInfoProviderPtr->mInterval,70000000);
    string result;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/container/all.json").string(),result);
    pContainerInfoProviderPtr->ParseDockerInspectInfos(result,pContainerInfoProviderPtr->mContainerInspectInfos);
    EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos.size(), size_t(22));
    if(pContainerInfoProviderPtr->mContainerInspectInfos.size()>1)
    {
        EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos[1].id,"618d7bdc433a6108df87d5bf4e7ea80c8706118b24e5987b493b9d83fc24616f");
        EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos[1].name,"paas-seed.SeedFlow__.seedflow.1606876691");
        EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos[1].mounts.size(), size_t(16));
        if(pContainerInfoProviderPtr->mContainerInspectInfos[1].mounts.size()== size_t(16))
        {
            EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos[1].mounts[6].source,"/cloud/app/tianji/TianjiClient#/tool/1939741");
            EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos[1].mounts[6].dest,"/cloud/tool/tianji");
        }
        EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos[1].labelMap["TJ_APP_NAME"],"seedflow");
    }
	pContainerInfoProviderPtr->mLastContainInspectInfoUpdate = NowMicros();
    vector<ContainerInspectInfo> containerInspectInfos;
    pContainerInfoProviderPtr->GetContainInspectInfos(containerInspectInfos);
    EXPECT_EQ(containerInspectInfos.size(), size_t(22));
    pContainerInfoProviderPtr->mLastContainInspectInfoUpdate = NowMicros()-pContainerInfoProviderPtr->mInterval-1000*1000;
    containerInspectInfos.clear();
    pContainerInfoProviderPtr->GetContainInspectInfos(containerInspectInfos);
    EXPECT_EQ(pContainerInfoProviderPtr->mContainerInspectInfos.size(), size_t(0));
    EXPECT_EQ(containerInspectInfos.size(), size_t(0));
}

TEST_F(CommonContainerInfoProviderTest,GetContainerNodePath)
{
    unique_ptr<ContainerInfoProvider> pContainerInfoProviderPtr(new ContainerInfoProvider());
    string result;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/container/all.json").string(),result);
    pContainerInfoProviderPtr->ParseDockerInspectInfos(result,pContainerInfoProviderPtr->mContainerInspectInfos);
    pContainerInfoProviderPtr->mLastContainInspectInfoUpdate = NowMicros();
    vector<string> dockerIds;
    dockerIds.push_back("618d7bdc433a6108df87d5bf4e7ea80c8706118b24e5987b493b9d83fc24616f");
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/run/test.log"));
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest/test.log"));
    EXPECT_EQ("",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest1/test.log"));
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test2/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest2/test.log"));
    dockerIds.clear();
    dockerIds.push_back("3775ea436031081f79487440e885800d814788f751fb4e17965c40db75619d12");
    dockerIds.push_back("618d7bdc433a6108df87d5bf4e7ea80c8706118b24e5987b493b9d83fc24616f");
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest/test.log"));
}

TEST_F(CommonContainerInfoProviderTest,GetContainerNodePath1)
{
    unique_ptr<ContainerInfoProvider> pContainerInfoProviderPtr(new ContainerInfoProvider());
    string result;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/container/all1.json").string(),result);
    pContainerInfoProviderPtr->ParseDockerInspectInfos(result,pContainerInfoProviderPtr->mContainerInspectInfos);
    pContainerInfoProviderPtr->mLastContainInspectInfoUpdate = NowMicros();
    vector<string> dockerIds;
    dockerIds.push_back("0216e6d73933fafa7a5fc338e10034bf0704eba914ce21c38072fa4d5b6518db");
    dockerIds.push_back("56118c90872de953f62519123352ec4eca0115770fedce50c7741ddd2493aa58");
    dockerIds.push_back("70e0fcd4e83ac4705a308dfb29189729903fffa86a86b579efe54f09df757f62");
    EXPECT_EQ("/var/lib/docker/logs/tianjimon.accagg/tianjimonaccagg/java/metricstore.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/alidata/www/logs/java/metricstore.log"));
}

TEST_F(CommonContainerInfoProviderTest,GetContainerNodePath3)
{
    unique_ptr<ContainerInfoProvider> pContainerInfoProviderPtr(new ContainerInfoProvider());
    string result;
    EXPECT_GT(FileUtils::ReadFileContent(TEST_CONF_PATH / "conf/container/all.json",result), 0);
    pContainerInfoProviderPtr->ParseDockerInspectInfos(result,pContainerInfoProviderPtr->mContainerInspectInfos);
    pContainerInfoProviderPtr->mLastContainInspectInfoUpdate = NowMicros();
    vector<string> dockerIds;
    ContainerNodePath containerNodePath;
    dockerIds.push_back("618d7bdc433a6108df87d5bf4e7ea80c8706118b24e5987b493b9d83fc24616f");
    EXPECT_TRUE(pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/run/test.log",containerNodePath));
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34/test.log",containerNodePath.file);
    EXPECT_EQ("/run",containerNodePath.mountInfo.dest);
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34",containerNodePath.mountInfo.source);
    //EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/run/test.log"));
    ContainerNodePath containerNodePath1;
    EXPECT_TRUE(pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest/test.log",containerNodePath1));
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test/test.log",containerNodePath1.file);
    EXPECT_EQ("/runtest",containerNodePath1.mountInfo.dest);
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test",containerNodePath1.mountInfo.source);
    //EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest/test.log"));
    ContainerNodePath containerNodePath2;
    EXPECT_FALSE(pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest1/test.log",containerNodePath2));
    //EXPECT_EQ("",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest1/test.log"));
    ContainerNodePath containerNodePath3;
    EXPECT_TRUE(pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest2/test.log",containerNodePath3));
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test2/test.log",containerNodePath3.file);
    EXPECT_EQ("/runtest2/",containerNodePath3.mountInfo.dest);
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test2/",containerNodePath3.mountInfo.source);
    //EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test2/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest2/test.log"));
    dockerIds.clear();
    dockerIds.push_back("3775ea436031081f79487440e885800d814788f751fb4e17965c40db75619d12");
    dockerIds.push_back("618d7bdc433a6108df87d5bf4e7ea80c8706118b24e5987b493b9d83fc24616f");
    ContainerNodePath containerNodePath4;
    EXPECT_TRUE(pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest/test.log",containerNodePath4));
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test/test.log",containerNodePath4.file);
    EXPECT_EQ("/runtest",containerNodePath4.mountInfo.dest);
    EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test",containerNodePath4.mountInfo.source);
    //EXPECT_EQ("/apsarapangu/disk5/vfsdir/4afeb93131f9f81c08f66e1443082b66c76769d016e7c00be3406acfdcd21e34test/test.log",pContainerInfoProviderPtr->GetContainerNodePath(dockerIds,"/runtest/test.log"));
}


TEST_F(CommonContainerInfoProviderTest,ParseContainerInfo)
{
    unique_ptr<ContainerInfoProvider> pContainerInfoProviderPtr(new ContainerInfoProvider());
    string result;
    FileUtils::ReadFileContent(TEST_CONF_PATH / "conf/container/one.json",result);
    ContainerInfo containerInfo;
    EXPECT_TRUE(pContainerInfoProviderPtr->ParseContainerInfo(result,containerInfo));
    EXPECT_EQ(containerInfo.id,"5e9494e0230da167460c93700375907825db3f443f6de0348167bf6ecde3b440");
    EXPECT_EQ(containerInfo.pid,"41130");
}

TEST_F(CommonContainerInfoProviderTest,GetContainerCgroupInfo)
{
    unique_ptr<ContainerInfoProvider> pContainerInfoProviderPtr(new ContainerInfoProvider());
    ContainerCgroupInfo  containerCgroupInfo;
    EXPECT_TRUE(pContainerInfoProviderPtr->GetContainerCgroupInfo((TEST_CONF_PATH / "conf/container/cgroup.json").string(),containerCgroupInfo));
    EXPECT_EQ(containerCgroupInfo.cpuacc,"/docker/5e9494e0230da167460c93700375907825db3f443f6de0348167bf6ecde3b440");
    EXPECT_EQ(containerCgroupInfo.memory,"/docker/testmemory");
    EXPECT_EQ(containerCgroupInfo.net_cls,"/docker/testcls");
    EXPECT_EQ(containerCgroupInfo.blkio,"/docker/testblkio");
}