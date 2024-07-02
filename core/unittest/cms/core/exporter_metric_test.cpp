#include  <gtest/gtest.h>
#include "core/exporter_metric.h"

#include "common/Logger.h"
#include "common/Config.h"
#include "common/SystemUtil.h"
#include "common/StringUtils.h"
#include "common/FileUtils.h"
#include "common/UnitTestEnv.h"

using namespace common;
using namespace argus;
using namespace std;

class ExporterMetricTest:public testing::Test{
protected:
   void SetUp() {

  }
void TearDown(){

  }
};
void ResetCommonMetric(CommonMetric &commonMetric)
{
  commonMetric.value =0;
  commonMetric.name="";
  commonMetric.tagMap.clear();
}
TEST_F(ExporterMetricTest,ParseLine){
   vector<MetricFilterInfo> metricFilterInfos;
   vector<LabelAddInfo> labelAddInfos;
   ExporterMetric *pMetric = new ExporterMetric(metricFilterInfos,labelAddInfos);
   CommonMetric commonMetric;
   string str = "container_cpu_load_average_10s{id=\"xxxx\",name=\"docker_ubuntu_16.10\"} 1.0";
   EXPECT_EQ(0,pMetric->ParseLine(str,commonMetric));
   EXPECT_EQ(commonMetric.name,"container_cpu_load_average_10s");
   EXPECT_EQ(commonMetric.value,1.0);
   EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
   EXPECT_EQ(commonMetric.tagMap["id"],"xxxx");
   EXPECT_EQ(commonMetric.tagMap["name"],"docker_ubuntu_16.10");
   ResetCommonMetric(commonMetric);
   str = "container_cpu_load_average_10s{id=\"xxxx\",name=\"docker_ubuntu_16.10\"} 1.0";
   EXPECT_EQ(0,pMetric->ParseLine(str,commonMetric));
   EXPECT_EQ(commonMetric.name,"container_cpu_load_average_10s");
   EXPECT_EQ(commonMetric.value,1.0);
   EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
   EXPECT_EQ(commonMetric.tagMap["id"],"xxxx");
   EXPECT_EQ(commonMetric.tagMap["name"],"docker_ubuntu_16.10");
   ResetCommonMetric(commonMetric);
   str = "container_cpu_load_average_10s 1.0";
   EXPECT_EQ(0,pMetric->ParseLine(str,commonMetric));
   EXPECT_EQ(commonMetric.name,"container_cpu_load_average_10s");
   EXPECT_EQ(commonMetric.value,1.0);
   EXPECT_EQ(commonMetric.tagMap.size(), size_t(0));
   ResetCommonMetric(commonMetric);
   str = "#container_cpu_load_average_10s 1.0";
   EXPECT_EQ(-1,pMetric->ParseLine(str,commonMetric));
   EXPECT_EQ(commonMetric.name.size(), size_t(0));
  ResetCommonMetric(commonMetric);
  str.clear();
  FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/exporter1.content").string(),str);
  LogInfo("{}", str);
  EXPECT_EQ(0,pMetric->ParseLine(str,commonMetric));
  EXPECT_EQ(commonMetric.name,"msdos_file_access_time_seconds");
  EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
  delete pMetric;
}

TEST_F(ExporterMetricTest,LabelAddInfo){
  vector<MetricFilterInfo> metricFilterInfos;
  vector<LabelAddInfo> labelAddInfos;
  LabelAddInfo labelAddInfo;
  labelAddInfo.name = "hostname";
  labelAddInfo.type = 0;
  labelAddInfos.push_back(labelAddInfo);
  labelAddInfo.name = "instance";
  labelAddInfo.type = 1;
  labelAddInfo.value = "INSTANCE";
  labelAddInfos.push_back(labelAddInfo);
  labelAddInfo.name = "test_add";
  labelAddInfo.type = 2;
  labelAddInfo.value = "test_add_label";
  labelAddInfos.push_back(labelAddInfo);
  labelAddInfo.name = "name";
  labelAddInfo.type = 3;
  labelAddInfo.value = "test_rename";
  labelAddInfos.push_back(labelAddInfo);
  SystemUtil::SetEnv("INSTANCE","TEST_INSTANCE");
  ExporterMetric *pMetric = new ExporterMetric(metricFilterInfos,labelAddInfos);
  string str = "container_cpu_load_average_1s {id=\"id1\",name=\" docker_ubuntu_16.10 \"} 1.0\n";
  str += "container_cpu_load_average_10s {id=\"id2\",name=\" docker_ubuntu_16.10\"} 2.0\n";
  pMetric->AddMetric(str);
  vector<CommonMetric> commonMetrics;
  EXPECT_EQ(2,pMetric->GetCommonMetrics(commonMetrics));
  for(size_t i=0;i<commonMetrics.size();i++)
  {
    EXPECT_EQ(commonMetrics[i].tagMap["hostname"],SystemUtil::getHostname());
    EXPECT_EQ(commonMetrics[i].tagMap["instance"],"TEST_INSTANCE");
    EXPECT_EQ(commonMetrics[i].tagMap["test_add"],"test_add_label");
    EXPECT_EQ(commonMetrics[i].tagMap["test_rename"],"docker_ubuntu_16.10");
  }
  delete pMetric;
}

TEST_F(ExporterMetricTest,AddMetric1){
  vector<MetricFilterInfo> metricFilterInfos;
  MetricFilterInfo metricFilterInfo;
  metricFilterInfo.name = "container_cpu_load_average_1s";
  metricFilterInfos.push_back(metricFilterInfo);
  metricFilterInfo.name = "container_cpu_load_average_10s";
  metricFilterInfos.push_back(metricFilterInfo);
  metricFilterInfo.name = "container_cpu_load_average_20s";
  metricFilterInfos.push_back(metricFilterInfo);
  vector<LabelAddInfo> labelAddInfos;
  ExporterMetric *pMetric = new ExporterMetric(metricFilterInfos,labelAddInfos);
  string str = "container_cpu_load_average_1s {id =\"id1\",name=\"docker_ubuntu_16.10\"} 1.0\n";
  str += "container_cpu_load_average_10s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 2.0\n";
  str += "container_cpu_load_average_20s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 2.0\n";
  pMetric->AddMetric(str);
  vector<CommonMetric> commonMetrics;
  EXPECT_EQ(3,pMetric->GetCommonMetrics(commonMetrics));
  str = "container_cpu_load_average_1s {id =\"id1\",name=\"docker_ubuntu_16.10\"} 1.0\n";
  str += "container_cpu_load_average_10s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 5.0\n";
  str += "container_cpu_load_average_20s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 4.0\n";
  pMetric->AddMetric(str);
  commonMetrics.clear();
  EXPECT_EQ(3,pMetric->GetCommonMetrics(commonMetrics));
  for(size_t i=0;i<commonMetrics.size();i++)
  {
    EXPECT_EQ(commonMetrics[i].tagMap.size(), size_t(2));
    if(commonMetrics[i].name=="container_cpu_load_average_1s")
    {
      EXPECT_EQ(commonMetrics[i].value,1.0);
    }else if(commonMetrics[i].name=="container_cpu_load_average_10s")
    {
      EXPECT_EQ(commonMetrics[i].value,5.0);
    }else if(commonMetrics[i].name=="container_cpu_load_average_20s")
    {
      EXPECT_EQ(commonMetrics[i].value,4.0);
    }
  }

  str = "container_cpu_load_average_1s {id =\"id1\",name=\"docker_ubuntu_16.10\"} 3.0\n";
  str += "container_cpu_load_average_10s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 4.0\n";
  str += "container_cpu_load_average_20s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 2.0\n";
  pMetric->AddMetric(str);
  commonMetrics.clear();
  EXPECT_EQ(3,pMetric->GetCommonMetrics(commonMetrics));
  for(size_t i=0;i<commonMetrics.size();i++)
  {
    EXPECT_EQ(commonMetrics[i].tagMap.size(), size_t(2));
    if(commonMetrics[i].name=="container_cpu_load_average_1s")
    {
      EXPECT_EQ(commonMetrics[i].value,3.0);
    }else if(commonMetrics[i].name=="container_cpu_load_average_10s")
    {
      EXPECT_EQ(commonMetrics[i].value,4.0);
    }else if(commonMetrics[i].name=="container_cpu_load_average_20s")
    {
      EXPECT_EQ(commonMetrics[i].value>1.90,true);
      EXPECT_EQ(commonMetrics[i].value<2.10,true);
    }
  }

  str = "container_cpu_load_average_1s {id =\"id1\",name=\"docker_ubuntu_16.10\"} 3.0\n";
  str += "container_cpu_load_average_10s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 5.0\n";
  str += "container_cpu_load_average_20s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 3.0\n";
  pMetric->AddMetric(str);
  commonMetrics.clear();
  EXPECT_EQ(3,pMetric->GetCommonMetrics(commonMetrics));
  for(size_t i=0;i<commonMetrics.size();i++)
  {
    EXPECT_EQ(commonMetrics[i].tagMap.size(), size_t(2));
    if(commonMetrics[i].name=="container_cpu_load_average_1s")
    {
      EXPECT_EQ(commonMetrics[i].value,3.0);
    }else if(commonMetrics[i].name=="container_cpu_load_average_10s")
    {
      EXPECT_EQ(commonMetrics[i].value,5.0);
    }else if(commonMetrics[i].name=="container_cpu_load_average_20s")
    {
      EXPECT_EQ(commonMetrics[i].value,3.0);
      cout<<commonMetrics[i].value<<endl;
    }
  }
  delete pMetric;
}

TEST_F(ExporterMetricTest,AddMetric2){
  vector<MetricFilterInfo> metricFilterInfos;
  MetricFilterInfo metricFilterInfo;
  metricFilterInfo.name = "container_cpu_load_average_1s";
  metricFilterInfo.tagMap["id"]="id1";
  metricFilterInfo.tagMap["name"]="docker_ubuntu_16.10";
  metricFilterInfos.push_back(metricFilterInfo);
  metricFilterInfo.name = "container_cpu_load_average_10s";
  metricFilterInfo.tagMap["id"]="id2";
  metricFilterInfo.tagMap["name"]="docker_ubuntu_16.100";
  metricFilterInfos.push_back(metricFilterInfo);
  vector<LabelAddInfo> labelAddInfos;
  ExporterMetric *pMetric = new ExporterMetric(metricFilterInfos,labelAddInfos);
  string str = "container_cpu_load_average_1s {id =\"id1\",name=\"docker_ubuntu_16.10\"} 1.0\n";
  str += "container_cpu_load_average_10s {id =\"id2\",name=\"docker_ubuntu_16.10\"} 2.0\n";
  str += "container_cpu_load_average_20s {id =\"id2\",name=\"docker_ubuntu_16.10\"} 2.0\n";
  pMetric->AddMetric(str);
  vector<CommonMetric> commonMetrics;
  EXPECT_EQ(1,pMetric->GetCommonMetrics(commonMetrics));

  str = "container_cpu_load_average_1s {id=\"id1\",name=\"docker_ubuntu_16.10\"} 1.0\n";
  str += "container_cpu_load_average_10s {id=\"id2\",name=\"docker_ubuntu_16.100\"} 5.0\n";
  str += "container_cpu_load_average_20s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 4.0\n";
  pMetric->AddMetric(str);
  commonMetrics.clear();
  EXPECT_EQ(2,pMetric->GetCommonMetrics(commonMetrics));
  for(size_t i=0;i<commonMetrics.size();i++)
  {
    EXPECT_EQ(commonMetrics[i].tagMap.size(), size_t(2));
  }
  if(commonMetrics.size()>1){
    EXPECT_EQ(commonMetrics[0].value,1.0);
    EXPECT_EQ(commonMetrics[1].value,5.0);
  }
  delete pMetric;
}

TEST_F(ExporterMetricTest,AddMetric3){
  vector<MetricFilterInfo> metricFilterInfos;
  MetricFilterInfo metricFilterInfo;
  metricFilterInfo.name = "container_cpu_load_average_1s";
  metricFilterInfo.tagMap["id"]="id1";
  metricFilterInfo.tagMap["name"]="docker_ubuntu_16.10";
  metricFilterInfo.metricName="cpu_1";
  metricFilterInfos.push_back(metricFilterInfo);
  metricFilterInfo.name = "container_cpu_load_average_10s";
  metricFilterInfo.tagMap["id"]="id2";
  metricFilterInfo.tagMap["name"]="docker_ubuntu_16.100";
  metricFilterInfo.metricName="cpu_10";
  metricFilterInfos.push_back(metricFilterInfo);
  vector<LabelAddInfo> labelAddInfos;
  ExporterMetric *pMetric = new ExporterMetric(metricFilterInfos,labelAddInfos);
  string str = "container_cpu_load_average_1s {id=\"id1\",name=\"docker_ubuntu_16.10\"} 1.0\n";
  str += "container_cpu_load_average_10s {id=\"id2\",name=\"docker_ubuntu_16.100\"} 5.0\n";
  str += "container_cpu_load_average_20s {id=\"id2\",name=\"docker_ubuntu_16.10\"} 4.0\n";
  pMetric->AddMetric(str);
  vector<CommonMetric> commonMetrics;
  EXPECT_EQ(2,pMetric->GetCommonMetrics(commonMetrics));
  for(size_t i=0;i<commonMetrics.size();i++)
  {
    EXPECT_EQ(commonMetrics[i].tagMap.size(), size_t(2));
  }
  if (commonMetrics.size() > 1) {
    EXPECT_EQ(commonMetrics[0].value,1.0);
    EXPECT_EQ(commonMetrics[0].name,"cpu_1");
    EXPECT_EQ(commonMetrics[1].value,5.0);
    EXPECT_EQ(commonMetrics[1].name,"cpu_10");
  }
  delete pMetric;
}
TEST_F(ExporterMetricTest,AddMetric4){
  vector<MetricFilterInfo> metricFilterInfos;
  MetricFilterInfo metricFilterInfo;
  vector<LabelAddInfo> labelAddInfos;
  ExporterMetric *pMetric = new ExporterMetric(metricFilterInfos,labelAddInfos);
  string str = "";
  EXPECT_EQ(pMetric->AddMetric(str),BaseParseMetric::EMPTY_STRING);
  delete pMetric;
}


