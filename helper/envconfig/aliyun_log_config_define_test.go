// Copyright 2021 iLogtail Authors
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

package envconfig

import (
	"fmt"
	"github.com/alibabacloud-go/tea/tea"
	"testing"

	"github.com/alibaba/ilogtail/pkg/flags"
	"github.com/alibaba/ilogtail/pkg/helper"
	_ "github.com/alibaba/ilogtail/pkg/logger/test"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/pingcap/check"
)

// go test -check.f "logConfigTestSuite.*" -args -debug=true -console=true
var _ = check.Suite(&logConfigTestSuite{})

func Test(t *testing.T) {
	check.TestingT(t)
}

func MockDockerInfoDetail(containerName string, envList []string) *helper.DockerInfoDetail {
	dockerInfo := types.ContainerJSON{}
	dockerInfo.ContainerJSONBase = &types.ContainerJSONBase{}
	dockerInfo.Name = containerName
	dockerInfo.Config = &container.Config{}
	dockerInfo.Config.Env = envList
	return helper.CreateContainerInfoDetail(dockerInfo, *flags.LogConfigPrefix, false)
}

type logConfigTestSuite struct {
}

func (s *logConfigTestSuite) SetUpSuite(c *check.C) {
}

func (s *logConfigTestSuite) TearDownSuite(c *check.C) {
}

func (s *logConfigTestSuite) SetUpTest(c *check.C) {

}

func (s *logConfigTestSuite) TearDownTest(c *check.C) {

}

func (s *logConfigTestSuite) TestNormal(c *check.C) {
	info := MockDockerInfoDetail("containerName", []string{
		"aliyun_logs_catalina=stdout",
	})
	c.Assert(info.EnvConfigInfoMap["catalina"], check.NotNil)
	c.Assert(len(info.EnvConfigInfoMap), check.Equals, 1)
	config := makeLogConfigSpec(info, info.EnvConfigInfoMap["catalina"])
	c.Assert(config.Project, check.Equals, *flags.DefaultLogProject)
	c.Assert(len(config.MachineGroups), check.Equals, 0)
	c.Assert(config.Logstore, check.Equals, "catalina")
	c.Assert(config.ShardCount, check.IsNil)
	c.Assert(config.LifeCycle, check.IsNil)
	c.Assert(config.SimpleConfig, check.Equals, true)

	c.Assert(len(config.LogtailConfig.Inputs[0]), check.Equals, 4)
	c.Assert(config.LogtailConfig.Inputs[0]["Type"], check.Equals, "service_docker_stdout")
	c.Assert(config.LogtailConfig.Inputs[0]["Stderr"].(bool), check.Equals, true)
	c.Assert(config.LogtailConfig.Inputs[0]["Stdout"], check.Equals, true)
	c.Assert(config.LogtailConfig.Inputs[0]["IncludeEnv"].(map[string]string)["aliyun_logs_catalina"], check.Equals, "stdout")
	c.Assert(config.LogtailConfig.Global["AlwaysOnline"], check.Equals, true)
	c.Assert(tea.StringValue(config.LogtailConfig.ConfigName), check.Equals, "catalina")
}

func (s *logConfigTestSuite) TestK8S(c *check.C) {
	info := MockDockerInfoDetail("k8s_logtail_logtail-ds-rq95g_kube-system_32417d70-9085-11e8-851d-00163f008685_0", []string{
		"aliyun_logs_catalina=stdout",
	})
	c.Assert(info.K8SInfo.ContainerName, check.Equals, "logtail")
	c.Assert(info.K8SInfo.Namespace, check.Equals, "kube-system")
	c.Assert(info.K8SInfo.Pod, check.Equals, "logtail-ds-rq95g")
	c.Assert(info.ContainerNameTag["_container_name_"], check.Equals, "logtail")
	c.Assert(info.ContainerNameTag["_namespace_"], check.Equals, "kube-system")
	c.Assert(info.ContainerNameTag["_pod_name_"], check.Equals, "logtail-ds-rq95g")
	c.Assert(info.EnvConfigInfoMap["catalina"], check.NotNil)
	c.Assert(len(info.EnvConfigInfoMap), check.Equals, 1)
	config := makeLogConfigSpec(info, info.EnvConfigInfoMap["catalina"])
	c.Assert(config.Project, check.Equals, *flags.DefaultLogProject)
	c.Assert(len(config.MachineGroups), check.Equals, 0)
	c.Assert(config.Logstore, check.Equals, "catalina")
	c.Assert(config.ShardCount, check.IsNil)
	c.Assert(config.LifeCycle, check.IsNil)
	c.Assert(config.SimpleConfig, check.Equals, true)

	c.Assert(len(config.LogtailConfig.Inputs[0]), check.Equals, 4)
	c.Assert(config.LogtailConfig.Inputs[0]["Type"], check.Equals, "service_docker_stdout")
	c.Assert(config.LogtailConfig.Inputs[0]["Stderr"].(bool), check.Equals, true)
	c.Assert(config.LogtailConfig.Inputs[0]["Stdout"], check.Equals, true)
	c.Assert(config.LogtailConfig.Inputs[0]["IncludeEnv"].(map[string]string)["aliyun_logs_catalina"], check.Equals, "stdout")
	c.Assert(config.LogtailConfig.Global["AlwaysOnline"], check.Equals, true)
	c.Assert(tea.StringValue(config.LogtailConfig.ConfigName), check.Equals, "catalina")
}

func (s *logConfigTestSuite) TestTags(c *check.C) {
	info := MockDockerInfoDetail("containerName", []string{
		"aliyun_logs_catalina=stdout",
		"aliyun_logs_catalina_tags=app=tomcat",
	})
	c.Assert(info.ContainerNameTag["app"], check.Equals, "tomcat")

}

func (s *logConfigTestSuite) TestMulti(c *check.C) {
	info := MockDockerInfoDetail("containerName", []string{
		"aliyun_logs_catalina=stdout",
		"aliyun_logs_filelogs=invalid-file-path",
	})
	{
		c.Assert(info.EnvConfigInfoMap["catalina"], check.NotNil)
		c.Assert(len(info.EnvConfigInfoMap), check.Equals, 2)
		config := makeLogConfigSpec(info, info.EnvConfigInfoMap["catalina"])
		c.Assert(config.Project, check.Equals, *flags.DefaultLogProject)
		c.Assert(len(config.MachineGroups), check.Equals, 0)
		c.Assert(config.Logstore, check.Equals, "catalina")
		c.Assert(config.ShardCount, check.IsNil)
		c.Assert(config.LifeCycle, check.IsNil)
		c.Assert(config.SimpleConfig, check.Equals, true)
		c.Assert(len(config.LogtailConfig.Inputs[0]), check.Equals, 4)
		c.Assert(config.LogtailConfig.Inputs[0]["Type"], check.Equals, "service_docker_stdout")
		c.Assert(config.LogtailConfig.Inputs[0]["Stderr"].(bool), check.Equals, true)
		c.Assert(config.LogtailConfig.Inputs[0]["Stdout"], check.Equals, true)
		c.Assert(config.LogtailConfig.Inputs[0]["IncludeEnv"].(map[string]string)["aliyun_logs_catalina"], check.Equals, "stdout")
		c.Assert(config.LogtailConfig.Global["AlwaysOnline"], check.Equals, true)
		c.Assert(tea.StringValue(config.LogtailConfig.ConfigName), check.Equals, "catalina")
	}

	{
		c.Assert(info.EnvConfigInfoMap["filelogs"], check.NotNil)
		c.Assert(len(info.EnvConfigInfoMap), check.Equals, 2)
		config := makeLogConfigSpec(info, info.EnvConfigInfoMap["filelogs"])
		c.Assert(config.Project, check.Equals, *flags.DefaultLogProject)
		c.Assert(len(config.MachineGroups), check.Equals, 0)
		c.Assert(config.Logstore, check.Equals, "filelogs")
		c.Assert(config.ShardCount, check.IsNil)
		c.Assert(config.LifeCycle, check.IsNil)
		c.Assert(config.SimpleConfig, check.Equals, true)

		c.Assert(len(config.LogtailConfig.Inputs[0]), check.Equals, 5)
		c.Assert(config.LogtailConfig.Inputs[0]["Type"], check.Equals, "input_file")
		c.Assert(config.LogtailConfig.Inputs[0]["FilePaths"].([]string)[0], check.Equals, invalidLogPath+"/**/"+invalidFilePattern)
		c.Assert(config.LogtailConfig.Inputs[0]["EnableContainerDiscovery"].(bool), check.Equals, true)
		c.Assert(config.LogtailConfig.Inputs[0]["ContainerFilters"].(map[string]map[string]interface{})["IncludeEnv"]["aliyun_logs_filelogs"], check.Equals, "invalid-file-path")
		c.Assert(config.LogtailConfig.Inputs[0]["MaxDirSearchDepth"].(int), check.Equals, 20)
		c.Assert(tea.StringValue(config.LogtailConfig.ConfigName), check.Equals, "filelogs")
	}
}

func (s *logConfigTestSuite) TestAllConfigs(c *check.C) {
	info := MockDockerInfoDetail("containerName", []string{
		"aliyun_logs_catalina=file",
		"aliyun_logs_catalina_project=my-project",
		"aliyun_logs_catalina_logstore=my-logstore",
		"aliyun_logs_catalina_shard=10",
		"aliyun_logs_catalina_ttl=3650",
		"aliyun_logs_catalina_machinegroup=my-group",
		"aliyun_logs_catalina_detail={\"configName\":\"envTest\",\"global\":{\"EnableTag\":true,\"ProcessPriority\":2,\"TopicType\":\"default\"},\"inputs\":[{\"Type\":\"input_file\",\"MaxDirSearchDepth\":0,\"EnableContainerDiscovery\":true,\"TailSizeKB\":1024,\"FilePaths\":[\"/python/logs/*.LOG\"],\"ExcludeFilePaths\":[\"/asdf/asdfasdfaesdf\",\"/asdf/asdfasdf34qrt\"],\"ExcludeDirs\":[\"/asdf/asdfasdf\",\"/asdf/asdfasdfasdadsf\"],\"ExternalK8sLabelTag\":{\"avd\":\"adsv\"},\"ExternalEnvTag\":{\"adsv\":\"avds\"},\"ContainerFilters\":{\"K8sNamespaceRegex\":\"asdf\",\"K8sPodRegex\":\"asdf\",\"K8sContainerRegex\":\"asdf\",\"IncludeK8sLabel\":{\"asdf\":\"adsf\"},\"ExcludeK8sLabel\":{\"adsf\":\"vads\"},\"IncludeEnv\":{\"aerv\":\"advf\",\"sb\":\"avergf\"},\"ExcludeEnv\":{\"adsvf\":\"adsv\",\"asdvc\":\"asdv\"},\"IncludeContainerLabel\":{\"aef\":\"asdf\",\"srabgth\":\"srtb\"},\"ExcludeContainerLabel\":{\"asdf\":\"xfgb\",\"sfvb\":\"sbfr\"}},\"PreservedDirDepth\":0}],\"processors\":[{\"Type\":\"processor_parse_apsara_native\",\"SourceKey\":\"content\",\"KeepingSourceWhenParseFail\":true,\"KeepingSourceWhenParseSucceed\":true,\"RenamedSourceKey\":\"__raw__\",\"AdjustingMicroTimezone\":false},{\"Type\":\"processor_filter_regex_native\",\"Include\":{\"a\":\"b\"}},{\"Type\":\"processor_desensitize_native\",\"SourceKey\":\"k1\",\"Method\":\"const\",\"ReplacingString\":\"******\",\"ContentPatternBeforeReplacedString\":\"password:\",\"ReplacedContentPattern\":\"\\\\\\\\d+\"}],\"flushers\":[{\"Type\":\"flusher_sls\",\"CompressType\":\"lz4\",\"Project\":\"test\",\"Logstore\":\"test\",\"TelemetryType\":\"logs\",\"Endpoint\":\"cn-shanghai-b-intranet.log.aliyuncs.com\",\"Region\":\"cn-shanghai-b\"}]}",
		"aliyun_logs_catalina_configtags={\"sls.logtail.creator\":\"test-user\", \"sls.logtail.group\":\"test-group\", \"sls.logtail.datasource\":\"k8s\", \"test-tag\":\"test-value\"}",
	})
	c.Assert(info.EnvConfigInfoMap["catalina"], check.NotNil)
	c.Assert(len(info.EnvConfigInfoMap), check.Equals, 1)
	config := makeLogConfigSpec(info, info.EnvConfigInfoMap["catalina"])
	c.Assert(config.Project, check.Equals, "my-project")
	c.Assert(len(config.MachineGroups), check.Equals, 1)
	c.Assert(config.MachineGroups[0], check.Equals, "my-group")
	c.Assert(config.Logstore, check.Equals, "my-logstore")
	c.Assert(*config.ShardCount, check.Equals, int32(10))
	c.Assert(*config.LifeCycle, check.Equals, int32(3650))
	c.Assert(config.SimpleConfig, check.Equals, false)

	c.Assert(len(config.LogtailConfig.Inputs[0]), check.Equals, 11)
	c.Assert(len(config.LogtailConfig.Processors), check.Equals, 3)
	c.Assert(len(config.LogtailConfig.Flushers), check.Equals, 1)
	c.Assert(len(config.LogtailConfig.Global), check.Equals, 3)
	c.Assert(tea.StringValue(config.LogtailConfig.ConfigName), check.Equals, "envTest")
	c.Assert(config.ConfigTags["sls.logtail.creator"], check.Equals, "test-user")
	c.Assert(config.ConfigTags["sls.logtail.group"], check.Equals, "test-group")
	c.Assert(config.ConfigTags["sls.logtail.datasource"], check.Equals, "k8s")
	c.Assert(config.ConfigTags["test-tag"], check.Equals, "test-value")
}

func (s *logConfigTestSuite) TestNginxIngress(c *check.C) {
	info := MockDockerInfoDetail("containerName", []string{
		"aliyun_logs_ingress-access-abc123_product=k8s-ingress-nginx",
		"aliyun_logs_ingress-access-abc123=stdout-only",
		"aliyun_logs_ingress-error-abc123=stderr-only",
	})
	{
		c.Assert(info.EnvConfigInfoMap["ingress-access-abc123"], check.NotNil)
		c.Assert(len(info.EnvConfigInfoMap), check.Equals, 2)
		config := makeLogConfigSpec(info, info.EnvConfigInfoMap["ingress-access-abc123"])
		fmt.Println("ingress access config : ", *(info.EnvConfigInfoMap["ingress-access-abc123"]))
		c.Assert(config.Project, check.Equals, *flags.DefaultLogProject)
		c.Assert(len(config.MachineGroups), check.Equals, 0)
		c.Assert(config.Logstore, check.Equals, "ingress-access-abc123")
		c.Assert(config.ShardCount, check.IsNil)
		c.Assert(config.LifeCycle, check.IsNil)
		c.Assert(config.SimpleConfig, check.Equals, true)

		c.Assert(len(config.LogtailConfig.Inputs[0]), check.Equals, 4)
		c.Assert(config.LogtailConfig.Inputs[0]["Type"], check.Equals, "service_docker_stdout")
		c.Assert(config.LogtailConfig.Inputs[0]["Stderr"].(bool), check.Equals, false)
		c.Assert(config.LogtailConfig.Inputs[0]["Stdout"], check.Equals, true)
		c.Assert(config.LogtailConfig.Inputs[0]["IncludeEnv"].(map[string]string)["aliyun_logs_ingress-access-abc123"], check.Equals, "stdout-only")
		c.Assert(config.LogtailConfig.Global["AlwaysOnline"], check.Equals, true)

		c.Assert(len(config.LogtailConfig.Processors[0]), check.Equals, 7)
		c.Assert(config.LogtailConfig.Processors[0]["Type"], check.Equals, "processor_regex")
		c.Assert(config.LogtailConfig.Processors[0]["SourceKey"], check.Equals, "content")
		c.Assert(config.LogtailConfig.Processors[0]["Regex"], check.Equals, "^(\\S+)\\s-\\s\\[([^]]+)]\\s-\\s(\\S+)\\s\\[(\\S+)\\s\\S+\\s\"(\\w+)\\s(\\S+)\\s([^\"]+)\"\\s(\\d+)\\s(\\d+)\\s\"([^\"]*)\"\\s\"([^\"]*)\"\\s(\\S+)\\s(\\S+)+\\s\\[([^]]*)]\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s(\\S+).*")
		c.Assert(config.LogtailConfig.Processors[0]["NoMatchError"], check.Equals, true)
		c.Assert(config.LogtailConfig.Processors[0]["NoKeyError"], check.Equals, true)
		c.Assert(config.LogtailConfig.Processors[0]["KeepSource"], check.Equals, true)
		expectedKeys := []interface{}{
			"client_ip",
			"x_forward_for",
			"remote_user",
			"time",
			"method",
			"url",
			"version",
			"status",
			"body_bytes_sent",
			"http_referer",
			"http_user_agent",
			"request_length",
			"request_time",
			"proxy_upstream_name",
			"upstream_addr",
			"upstream_response_length",
			"upstream_response_time",
			"upstream_status",
			"req_id",
		}
		keys := config.LogtailConfig.Processors[0]["Keys"].([]interface{})
		c.Assert(len(expectedKeys), check.Equals, len(keys))
		for i := 0; i < len(expectedKeys); i++ {
			c.Assert(keys[i], check.Equals, expectedKeys[i])
		}

		c.Assert(tea.StringValue(config.LogtailConfig.ConfigName), check.Equals, "ingress-access-abc123")
	}
	{
		c.Assert(info.EnvConfigInfoMap["ingress-error-abc123"], check.NotNil)
		c.Assert(len(info.EnvConfigInfoMap), check.Equals, 2)
		config := makeLogConfigSpec(info, info.EnvConfigInfoMap["ingress-error-abc123"])
		c.Assert(config.Project, check.Equals, *flags.DefaultLogProject)
		c.Assert(len(config.MachineGroups), check.Equals, 0)
		c.Assert(config.Logstore, check.Equals, "ingress-error-abc123")
		c.Assert(config.ShardCount, check.IsNil)
		c.Assert(config.LifeCycle, check.IsNil)
		c.Assert(config.SimpleConfig, check.Equals, true)

		c.Assert(len(config.LogtailConfig.Inputs[0]), check.Equals, 4)
		c.Assert(config.LogtailConfig.Inputs[0]["Type"], check.Equals, "service_docker_stdout")
		c.Assert(config.LogtailConfig.Inputs[0]["Stderr"].(bool), check.Equals, true)
		c.Assert(config.LogtailConfig.Inputs[0]["Stdout"], check.Equals, false)
		c.Assert(config.LogtailConfig.Inputs[0]["IncludeEnv"].(map[string]string)["aliyun_logs_ingress-error-abc123"], check.Equals, "stderr-only")
		c.Assert(config.LogtailConfig.Global["AlwaysOnline"], check.Equals, true)

		c.Assert(tea.StringValue(config.LogtailConfig.ConfigName), check.Equals, "ingress-error-abc123")
	}
}
