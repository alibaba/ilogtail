package manager

//func CreateOrUpdateAgentInstanceConfigs(agentInstanceConfigs []*entity.AgentInstanceConfig) error {
//	conflictColumnNames := []string{"agent_instance_id", "instance_config_name"}
//	assignmentColumnNames := []string{"status", "message"}
//	err := repository.CreateOrUpdateAgentInstanceConfigs(conflictColumnNames, assignmentColumnNames, agentInstanceConfigs...)
//	return common.SystemError(err)
//}
//
//func CreateOrUpdateAgentPipelineConfigs(agentPipelineConfigs []*entity.AgentPipelineConfig) error {
//	conflictColumnNames := []string{"agent_instance_id", "pipeline_config_name"}
//	assignmentColumnNames := []string{"status", "message"}
//	err := repository.CreateOrUpdateAgentPipelineConfigs(conflictColumnNames, assignmentColumnNames, agentPipelineConfigs...)
//	return common.SystemError(err)
//}
