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
	"github.com/alibaba/ilogtail/pkg/config"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	hostNameDefaultTagKey    = "host.name"
	hostIPDefaultTagKey      = "host.ip"
	defaultConfigTagKeyValue = "__default__"
)

// Processor interface cannot meet the requirements of tag processing, so we need to create a special ProcessorTag struct
type ProcessorTag struct {
	PipelineMetaTagKey           map[string]string
	EnableAgentEnvMetaTagControl bool
	AgentEnvMetaTagKey           map[string]string
}

func (p *ProcessorTag) ProcessV1(logCtx *pipeline.LogWithContext, globalConfig *config.GlobalConfig) {
	tags, ok := logCtx.Context["tags"]
	if !ok {
		return
	}
	tagsMap, ok := tags.(map[string]string)
	if !ok {
		return
	}
	p.addTagIfRequired("HOST_NAME", hostNameDefaultTagKey, util.GetHostName(), tagsMap)
	p.addTagIfRequired("HOST_IP", hostIPDefaultTagKey, util.GetIPAddress(), tagsMap)

	// Add tags for each log, includes: default hostname tag,
	// env tags and global tags in config.
	for k, v := range loadAdditionalTags(globalConfig).Iterator() {
		tagsMap[k] = v
	}
}

func (p *ProcessorTag) ProcessV2(in *models.PipelineGroupEvents, globalConfig *config.GlobalConfig) {
	tagsMap := make(map[string]string)
	p.addTagIfRequired("HOST_NAME", hostNameDefaultTagKey, util.GetHostName(), tagsMap)
	p.addTagIfRequired("HOST_IP", hostIPDefaultTagKey, util.GetIPAddress(), tagsMap)
	for k, v := range tagsMap {
		in.Group.Tags.Add(k, v)
	}

	// Add tags for each log, includes: default hostname tag,
	// env tags and global tags in config.
	for k, v := range loadAdditionalTags(globalConfig).Iterator() {
		in.Group.Tags.Add(k, v)
	}
}

func (p *ProcessorTag) addTagIfRequired(configKey, defaultKey, value string, tags map[string]string) {
	if key, ok := p.PipelineMetaTagKey[configKey]; ok {
		if key != "" {
			if key == defaultConfigTagKeyValue {
				tags[defaultKey] = value
			} else {
				tags[key] = value
			}
		}
	} else {
		tags[defaultKey] = value
	}
}
