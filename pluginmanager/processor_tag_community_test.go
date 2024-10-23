// Copyright 2024 iLogtail Authors
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

//go:build !enterprise

package pluginmanager

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"github.com/alibaba/ilogtail/pkg/config"
	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

func TestTagDefault(t *testing.T) {
	helper.EnvTags = []string{
		"test_env_tag",
		"test_env_tag_value",
	}
	processorTag := ProcessorTag{
		PipelineMetaTagKey:           map[string]string{},
		EnableAgentEnvMetaTagControl: false,
		AgentEnvMetaTagKey:           map[string]string{},
	}
	logCtx := &pipeline.LogWithContext{
		Context: map[string]interface{}{
			"tags": map[string]string{},
		},
	}
	globalConfig := &config.GlobalConfig{}
	processorTag.ProcessV1(logCtx, globalConfig)
	tagsMap := logCtx.Context["tags"].(map[string]string)
	assert.Equal(t, 3, len(tagsMap))
	assert.Equal(t, util.GetHostName(), tagsMap["host.name"])
	assert.Equal(t, util.GetIPAddress(), tagsMap["host.ip"])
	assert.Equal(t, "test_env_tag_value", tagsMap["test_env_tag"])

	processorTag.PipelineMetaTagKey = map[string]string{
		"HOST_NAME": "__default__",
		"HOST_IP":   "__default__",
	}
	logCtx = &pipeline.LogWithContext{
		Context: map[string]interface{}{
			"tags": map[string]string{},
		},
	}
	processorTag.ProcessV1(logCtx, globalConfig)
	tagsMap = logCtx.Context["tags"].(map[string]string)
	assert.Equal(t, 3, len(tagsMap))
	assert.Equal(t, util.GetHostName(), tagsMap["host.name"])
	assert.Equal(t, util.GetIPAddress(), tagsMap["host.ip"])
	assert.Equal(t, "test_env_tag_value", tagsMap["test_env_tag"])
}

func TestTagDefaultV2(t *testing.T) {
	helper.EnvTags = []string{
		"test_env_tag",
		"test_env_tag_value",
	}
	processorTag := ProcessorTag{
		PipelineMetaTagKey:           map[string]string{},
		EnableAgentEnvMetaTagControl: false,
		AgentEnvMetaTagKey:           map[string]string{},
	}
	in := &models.PipelineGroupEvents{
		Group: &models.GroupInfo{
			Tags: models.NewTags(),
		},
	}
	globalConfig := &config.GlobalConfig{}
	processorTag.ProcessV2(in, globalConfig)
	assert.Equal(t, util.GetHostName(), in.Group.Tags.Get("host.name"))
	assert.Equal(t, util.GetIPAddress(), in.Group.Tags.Get("host.ip"))
	assert.Equal(t, "test_env_tag_value", in.Group.Tags.Get("test_env_tag"))

	processorTag.PipelineMetaTagKey = map[string]string{
		"HOST_NAME": "__default__",
		"HOST_IP":   "__default__",
	}
	in = &models.PipelineGroupEvents{
		Group: &models.GroupInfo{
			Tags: models.NewTags(),
		},
	}
	processorTag.ProcessV2(in, globalConfig)
	assert.Equal(t, util.GetHostName(), in.Group.Tags.Get("host.name"))
	assert.Equal(t, util.GetIPAddress(), in.Group.Tags.Get("host.ip"))
	assert.Equal(t, "test_env_tag_value", in.Group.Tags.Get("test_env_tag"))
}

func TestTagRename(t *testing.T) {
	helper.EnvTags = []string{
		"test_env_tag",
		"test_env_tag_value",
	}
	processorTag := ProcessorTag{
		PipelineMetaTagKey: map[string]string{
			"HOST_NAME": "test_host_name",
			"HOST_IP":   "test_host_ip",
		},
		EnableAgentEnvMetaTagControl: false,
		AgentEnvMetaTagKey:           map[string]string{},
	}
	logCtx := &pipeline.LogWithContext{
		Context: map[string]interface{}{
			"tags": map[string]string{},
		},
	}
	globalConfig := &config.GlobalConfig{}
	processorTag.ProcessV1(logCtx, globalConfig)
	tagsMap := logCtx.Context["tags"].(map[string]string)
	assert.Equal(t, 3, len(tagsMap))
	assert.Equal(t, util.GetHostName(), tagsMap["test_host_name"])
	assert.Equal(t, util.GetIPAddress(), tagsMap["test_host_ip"])
	assert.Equal(t, "test_env_tag_value", tagsMap["test_env_tag"])
}

func TestTagRenameV2(t *testing.T) {
	helper.EnvTags = []string{
		"test_env_tag",
		"test_env_tag_value",
	}
	processorTag := ProcessorTag{
		PipelineMetaTagKey: map[string]string{
			"HOST_NAME": "test_host_name",
			"HOST_IP":   "test_host_ip",
		},
		EnableAgentEnvMetaTagControl: false,
		AgentEnvMetaTagKey:           map[string]string{},
	}
	in := &models.PipelineGroupEvents{
		Group: &models.GroupInfo{
			Tags: models.NewTags(),
		},
	}
	globalConfig := &config.GlobalConfig{}
	processorTag.ProcessV2(in, globalConfig)
	assert.Equal(t, util.GetHostName(), in.Group.Tags.Get("test_host_name"))
	assert.Equal(t, util.GetIPAddress(), in.Group.Tags.Get("test_host_ip"))
}

func TestTagDelete(t *testing.T) {
	helper.EnvTags = []string{
		"test_env_tag",
		"test_env_tag_value",
	}
	processorTag := ProcessorTag{
		PipelineMetaTagKey: map[string]string{
			"HOST_NAME": "",
			"HOST_IP":   "",
		},
		EnableAgentEnvMetaTagControl: false,
		AgentEnvMetaTagKey:           map[string]string{},
	}
	logCtx := &pipeline.LogWithContext{
		Context: map[string]interface{}{
			"tags": map[string]string{},
		},
	}
	globalConfig := &config.GlobalConfig{}
	processorTag.ProcessV1(logCtx, globalConfig)
	tagsMap := logCtx.Context["tags"].(map[string]string)
	assert.Equal(t, 1, len(tagsMap))
	assert.Equal(t, "test_env_tag_value", tagsMap["test_env_tag"])
}

func TestTagDeleteV2(t *testing.T) {
	helper.EnvTags = []string{
		"test_env_tag",
		"test_env_tag_value",
	}
	processorTag := ProcessorTag{
		PipelineMetaTagKey: map[string]string{
			"HOST_NAME": "",
			"HOST_IP":   "",
		},
		EnableAgentEnvMetaTagControl: false,
		AgentEnvMetaTagKey:           map[string]string{},
	}
	in := &models.PipelineGroupEvents{
		Group: &models.GroupInfo{
			Tags: models.NewTags(),
		},
	}
	globalConfig := &config.GlobalConfig{}
	processorTag.ProcessV2(in, globalConfig)
	assert.Equal(t, "", in.Group.Tags.Get("HOST_NAME"))
	assert.Equal(t, "", in.Group.Tags.Get("HOST_IP"))
	assert.Equal(t, "test_env_tag_value", in.Group.Tags.Get("test_env_tag"))
}
