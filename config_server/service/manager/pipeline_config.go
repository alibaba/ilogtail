package manager

import (
	"config-server/common"
	"config-server/entity"
	proto "config-server/protov2"
	"config-server/repository"
	"config-server/utils"
)

func SavePipelineConfigStatus(configs []*proto.ConfigInfo, instanceId string) error {
	if configs == nil {
		return nil
	}
	agent := &entity.Agent{
		InstanceId: instanceId,
	}
	for _, pipelineConfig := range configs {
		agent.PipelineConfigStatuses =
			append(agent.PipelineConfigStatuses, entity.ParseProtoPipelineConfigStatus2PipelineConfigStatus(pipelineConfig))
	}

	err := repository.UpdateAgentById(agent, "pipeline_config_statuses")
	return common.SystemError(err)
}

//configNames：需要的config ;instanceId:用于查询agent在哪些group中，最终查询它拥有的config

func GetPipelineConfigs(instanceId string, configInfos []*proto.ConfigInfo, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = repository.GetPipelineConfigsByAgent(agent)
	if err != nil {
		return nil, err
	}

	configUpdates := make([]*entity.PipelineConfig, 0)
	configEqFunc := func(a *entity.PipelineConfig, b *entity.PipelineConfig) bool {
		return a.Name == b.Name
	}
	configInfoEqFunc := func(a *proto.ConfigInfo, b *entity.PipelineConfig) bool {
		return a.Name == b.Name && a.Version == b.Version
	}

	for _, tag := range agent.Tags {
		for _, config := range tag.PipelineConfigs {
			//存在某些组里有重复配置的情况，需进行剔除；配置没更新（即版本没变化的）也无需加入数组
			if !utils.ContainElement(configInfos, config, configInfoEqFunc) &&
				!utils.ContainElement(configUpdates, config, configEqFunc) {
				configUpdates = append(configUpdates, config)
			}
		}
	}

	return utils.Map(configUpdates, func(config *entity.PipelineConfig) *proto.ConfigDetail {
		return (*config).Parse2ProtoPipelineConfigDetail(isContainDetail)
	}), nil
}
