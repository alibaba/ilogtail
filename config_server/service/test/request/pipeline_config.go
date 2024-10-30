package request

import (
	"config-server/protov2"
	"fmt"
)

func CreatePipelineConfig(requestId []byte, detail *protov2.ConfigDetail) (*protov2.CreateConfigResponse, error) {
	url := fmt.Sprintf("%s/CreatePipelineConfig", UserUrlPrefix)
	req := &protov2.CreateConfigRequest{}
	res := &protov2.CreateConfigResponse{}
	req.RequestId = requestId
	req.ConfigDetail = detail

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func GetPipelineConfig(requestId []byte, name string) (*protov2.GetConfigResponse, error) {
	url := fmt.Sprintf("%s/GetPipelineConfig", UserUrlPrefix)
	req := &protov2.GetConfigRequest{}
	res := &protov2.GetConfigResponse{}
	req.RequestId = requestId
	req.ConfigName = name

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func GetAllPipelineConfig(requestId []byte) (*protov2.ListConfigsResponse, error) {
	url := fmt.Sprintf("%s/ListPipelineConfigs", UserUrlPrefix)
	req := &protov2.ListConfigsRequest{}
	res := &protov2.ListConfigsResponse{}
	req.RequestId = requestId

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func UpdatePipelineConfig(requestId []byte, detail *protov2.ConfigDetail) (*protov2.UpdateConfigResponse, error) {
	url := fmt.Sprintf("%s/UpdatePipelineConfig", UserUrlPrefix)
	req := &protov2.UpdateConfigRequest{}
	res := &protov2.UpdateConfigResponse{}
	req.RequestId = requestId
	req.ConfigDetail = detail

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func DeletePipelineConfig(requestId []byte, name string) (*protov2.DeleteConfigResponse, error) {
	url := fmt.Sprintf("%s/DeletePipelineConfig", UserUrlPrefix)
	req := &protov2.DeleteConfigRequest{}
	res := &protov2.DeleteConfigResponse{}
	req.RequestId = requestId
	req.ConfigName = name

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func ApplyPipelineConfig2AgentGroup(requestId []byte, configName string, agentGroupName string) (*protov2.ApplyConfigToAgentGroupResponse, error) {
	url := fmt.Sprintf("%s/ApplyPipelineConfigToAgentGroup", UserUrlPrefix)
	req := &protov2.ApplyConfigToAgentGroupRequest{}
	res := &protov2.ApplyConfigToAgentGroupResponse{}
	req.RequestId = requestId
	req.ConfigName = configName
	req.GroupName = agentGroupName

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func RemovePipelineConfigFromAgentGroup(requestId []byte, configName string, groupName string) (*protov2.RemoveConfigFromAgentGroupResponse, error) {
	url := fmt.Sprintf("%s/RemovePipelineConfigFromAgentGroup", UserUrlPrefix)
	req := &protov2.RemoveConfigFromAgentGroupRequest{}
	res := &protov2.RemoveConfigFromAgentGroupResponse{}
	req.RequestId = requestId
	req.ConfigName = configName
	req.GroupName = groupName

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func GetAppliedAgentGroupForPipelineConfig(requestId []byte, configName string) (*protov2.GetAppliedAgentGroupsResponse, error) {
	url := fmt.Sprintf("%s/GetAppliedAgentGroupsWithPipelineConfig", UserUrlPrefix)
	req := &protov2.GetAppliedAgentGroupsRequest{}
	res := &protov2.GetAppliedAgentGroupsResponse{}
	req.RequestId = requestId
	req.ConfigName = configName

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}
