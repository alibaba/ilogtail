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

package configmanager

import (
	"fmt"
	"log"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	proto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (c *ConfigManager) updateConfigList(interval int) {
	ticker := time.NewTicker(time.Duration(interval) * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		s := store.GetStore()
		configList, getAllErr := s.GetAll(common.TypeConfigDetail)
		if getAllErr != nil {
			log.Println(getAllErr)
			continue
		} else {
			c.ConfigListMutex.Lock()
			c.ConfigList = make(map[string]*model.ConfigDetail, 0)
			for _, config := range configList {
				c.ConfigList[config.(*model.ConfigDetail).Name] = config.(*model.ConfigDetail)
			}
			c.ConfigListMutex.Unlock()
		}
	}
}

func (c *ConfigManager) checkConfigs(configType proto.ConfigType, configs []*proto.ConfigInfo, selectedConfigs map[string]*model.ConfigDetail) []*proto.ConfigCheckResult {
	results := make([]*proto.ConfigCheckResult, 0)
	// deleted or modified configs, and delete same configs in SelectedConfigs
	for _, k := range configs {
		config, ok := selectedConfigs[k.Name]
		if !ok {
			result := new(proto.ConfigCheckResult)
			result.Type = configType
			result.Name = k.Name
			result.OldVersion = k.Version
			result.NewVersion = k.Version
			result.Context = k.Context
			result.CheckStatus = proto.CheckStatus_DELETED
			results = append(results, result)
		} else if ok {
			if config.Version > k.Version {
				result := new(proto.ConfigCheckResult)
				result.Type = configType
				result.Name = config.Name
				result.OldVersion = k.Version
				result.NewVersion = config.Version
				result.Context = config.Context
				result.CheckStatus = proto.CheckStatus_MODIFIED
				results = append(results, result)
			}
			delete(selectedConfigs, k.Name)
		}
	}

	// new configs
	for _, config := range selectedConfigs {
		result := new(proto.ConfigCheckResult)
		result.Type = configType
		result.Name = config.Name
		result.OldVersion = 0
		result.NewVersion = config.Version
		result.Context = config.Context
		result.CheckStatus = proto.CheckStatus_NEW
		results = append(results, result)
	}

	return results
}

func (c *ConfigManager) CheckConfigUpdatesWhenHeartbeat(req *proto.HeartBeatRequest, res *proto.HeartBeatResponse) (int, *proto.HeartBeatResponse) {
	s := store.GetStore()

	// get all configs connected to agent group whose tag is same as agent's
	SelectedConfigs := make(map[string]*model.ConfigDetail)
	agentGroupList, getAllErr := s.GetAll(common.TypeAgentGROUP)
	if getAllErr != nil {
		res.Code = proto.RespCode_INTERNAL_SERVER_ERROR
		res.Message = getAllErr.Error()
		return common.InternalServerError.Status, res
	}

	for _, v := range agentGroupList {
		group := v.(*model.AgentGroup)
		agentTags := make([]model.AgentGroupTag, 0)
		for _, pt := range req.Tags {
			tag := new(model.AgentGroupTag)
			tag.ParseProto(pt)
			agentTags = append(agentTags, *tag)
		}
		if group.Name == "default" || c.tagsMatch(group.Tags, agentTags, group.TagOperator) {
			for k := range group.AppliedConfigs {
				// Check if config k exist
				config, hasErr := c.ConfigList[k]
				if !hasErr {
					res.Code = proto.RespCode_INVALID_PARAMETER
					res.Message = fmt.Sprintf("Config %s doesn't exist.", k)
					return common.ConfigNotExist.Status, res
				}

				if config.DelTag {
					res.Code = proto.RespCode_INVALID_PARAMETER
					res.Message = fmt.Sprintf("Config %s doesn't exist.", k)
					return common.ConfigNotExist.Status, res
				}

				SelectedConfigs[k] = config
			}
		}
	}

	agentCheckResults := c.checkConfigs(proto.ConfigType_AGENT_CONFIG, req.AgentConfigs, SelectedConfigs)
	pipelineCheckResults := c.checkConfigs(proto.ConfigType_PIPELINE_CONFIG, req.PipelineConfigs, SelectedConfigs)

	res.Code = proto.RespCode_ACCEPT
	res.Message += "Get config update infos success"
	res.AgentCheckResults = agentCheckResults
	res.PipelineCheckResults = pipelineCheckResults
	return common.Accept.Status, res
}

func (c *ConfigManager) FetchAgentConfig(req *proto.FetchAgentConfigRequest, res *proto.FetchAgentConfigResponse) (int, *proto.FetchAgentConfigResponse) {
	ans := make([]*proto.ConfigDetail, 0)

	// do something about attributes

	for _, configInfo := range req.ReqConfigs {
		config, ok := c.ConfigList[configInfo.Name]
		if !ok {
			res.Code = proto.RespCode_INVALID_PARAMETER
			res.Message += fmt.Sprintf("Config %s doesn't exist.\n", configInfo.Name)
			continue
		}

		if config.DelTag {
			res.Code = proto.RespCode_INVALID_PARAMETER
			res.Message = fmt.Sprintf("Config %s doesn't exist.\n", configInfo.Name)
			continue
		}
		if config.Type == configInfo.Type.String() {
			ans = append(ans, config.ToProto())
		}
	}

	res.Code = proto.RespCode_ACCEPT
	res.Message = "Get Agent Config details success"
	res.ConfigDetails = ans
	return common.Accept.Status, res
}

func (c *ConfigManager) FetchPipelineConfig(req *proto.FetchPipelineConfigRequest, res *proto.FetchPipelineConfigResponse) (int, *proto.FetchPipelineConfigResponse) {
	ans := make([]*proto.ConfigDetail, 0)

	for _, configInfo := range req.ReqConfigs {
		config, ok := c.ConfigList[configInfo.Name]
		if !ok {
			res.Code = proto.RespCode_INVALID_PARAMETER
			res.Message += fmt.Sprintf("Config %s doesn't exist.\n", configInfo.Name)
			continue
		}

		if config.DelTag {
			res.Code = proto.RespCode_INVALID_PARAMETER
			res.Message = fmt.Sprintf("Config %s doesn't exist.\n", configInfo.Name)
			continue
		}
		if config.Type == configInfo.Type.String() {
			ans = append(ans, config.ToProto())
		}
	}

	res.Code = proto.RespCode_ACCEPT
	res.Message = "Get Agent Config details success"
	res.ConfigDetails = ans
	return common.Accept.Status, res
}
