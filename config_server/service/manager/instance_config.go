package manager

import (
	"config-server/common"
	"config-server/entity"
	proto "config-server/protov2"
	"config-server/repository"
	"config-server/utils"
)

func SaveInstanceConfigStatus(configs []*proto.ConfigInfo, instanceId string) error {
	if configs == nil {
		return nil
	}
	agent := &entity.Agent{
		InstanceId: instanceId,
	}
	for _, instanceConfig := range configs {
		agent.InstanceConfigStatuses =
			append(agent.InstanceConfigStatuses, entity.ParseProtoInstanceConfigStatus2InstanceConfigStatus(instanceConfig))
	}

	err := repository.UpdateAgentById(agent, "instance_config_statuses")
	return common.SystemError(err)
}

func GetInstanceConfigs(instanceId string, configInfos []*proto.ConfigInfo, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = repository.GetInstanceConfigsByAgent(agent)
	if err != nil {
		return nil, err
	}

	configUpdates := make([]*entity.InstanceConfig, 0)
	configEqFunc := func(a *entity.InstanceConfig, b *entity.InstanceConfig) bool {
		return a.Name == b.Name
	}
	configInfoEqFunc := func(a *proto.ConfigInfo, b *entity.InstanceConfig) bool {
		return a.Name == b.Name && a.Version == b.Version
	}

	for _, tag := range agent.Tags {
		for _, config := range tag.InstanceConfigs {
			//存在某些组里有重复配置的情况，需进行剔除；配置没更新（即版本没变化的）也无需加入数组
			if !utils.ContainElement(configInfos, config, configInfoEqFunc) &&
				!utils.ContainElement(configUpdates, config, configEqFunc) {
				configUpdates = append(configUpdates, config)
			}
		}
	}

	return utils.Map(configUpdates, func(config *entity.InstanceConfig) *proto.ConfigDetail {
		return (*config).Parse2ProtoInstanceConfigDetail(isContainDetail)
	}), nil
}
