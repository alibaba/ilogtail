package entity

import (
	proto "config-server/protov2"
	"database/sql/driver"
	"encoding/json"
	"fmt"
)

type AgentAttributes struct {
	Version  []byte
	Ip       []byte
	Hostname []byte
	Extras   map[string][]byte
}

func (a *AgentAttributes) Parse2Proto() *proto.AgentAttributes {
	protoAgentAttributes := new(proto.AgentAttributes)
	protoAgentAttributes.Version = a.Version
	protoAgentAttributes.Ip = a.Ip
	protoAgentAttributes.Hostname = a.Hostname
	protoAgentAttributes.Extras = a.Extras
	return protoAgentAttributes
}

func ParseProtoAgentAttributes2AgentAttributes(attributes *proto.AgentAttributes) *AgentAttributes {
	agentAttributes := new(AgentAttributes)
	agentAttributes.Version = attributes.Version
	agentAttributes.Ip = attributes.Ip
	agentAttributes.Hostname = attributes.Hostname
	agentAttributes.Extras = attributes.Extras
	return agentAttributes
}

func (a *AgentAttributes) Scan(value any) error {
	if value == nil {
		return nil
	}

	b, ok := value.([]byte)
	if !ok {
		return fmt.Errorf("value is not []byte, value: %v", value)
	}

	return json.Unmarshal(b, a)
}

func (a *AgentAttributes) Value() (driver.Value, error) {
	v, err := json.Marshal(a)
	if err != nil {
		return nil, err
	}
	return v, nil
}

// Preload should write the name of the structure associated field, not the name of the data table or the name of the associated model structure

type Agent struct {
	SequenceNum            uint64
	Capabilities           uint64
	InstanceId             string `gorm:"primarykey"`
	AgentType              string
	Attributes             *AgentAttributes
	Tags                   []*AgentGroup `gorm:"many2many:agent_and_agent_group;foreignKey:InstanceId;joinForeignKey:AgentInstanceId;References:Name;joinReferences:AgentGroupName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
	RunningStatus          string
	StartupTime            int64
	PipelineConfigStatuses PipelineConfigStatusList
	InstanceConfigStatuses InstanceConfigStatusList
	//CustomCommands  []*CommandInfo
	Flags             uint64
	Opaque            []byte
	LastHeartBeatTime int64
}

func (a Agent) Parse2Proto() *proto.Agent {
	protoAgent := new(proto.Agent)
	protoAgent.Capabilities = a.Capabilities
	protoAgent.InstanceId = []byte(a.InstanceId)
	protoAgent.AgentType = a.AgentType
	protoAgent.Attributes = a.Attributes.Parse2Proto()
	protoAgent.RunningStatus = a.RunningStatus
	protoAgent.StartupTime = a.StartupTime
	protoAgent.PipelineConfigs = a.PipelineConfigStatuses.Parse2ProtoConfigStatus()
	protoAgent.InstanceConfigs = a.InstanceConfigStatuses.Parse2ProtoConfigStatus()
	return protoAgent
}

// ParseHeartBeatRequest2BasicAgent transfer agent's basic info
func ParseHeartBeatRequest2BasicAgent(req *proto.HeartbeatRequest, lastHeartBeatTime int64) *Agent {
	agent := new(Agent)
	agent.SequenceNum = req.SequenceNum
	agent.Capabilities = req.Capabilities
	agent.InstanceId = string(req.InstanceId)
	agent.AgentType = req.AgentType

	if req.Tags != nil {
		for _, tag := range req.Tags {
			agent.Tags = append(agent.Tags, ParseProtoAgentGroupTag2AgentGroup(tag))
		}
	}

	agent.RunningStatus = req.RunningStatus
	agent.StartupTime = req.StartupTime
	agent.Flags = req.Flags
	agent.Opaque = req.Opaque
	agent.LastHeartBeatTime = lastHeartBeatTime
	return agent
}

func (Agent) TableName() string {
	return agentTable
}
