package request

import (
	"config-server/protov2"
	"fmt"
)

func DeleteAgentGroup(requestId []byte, groupName string) (*protov2.DeleteAgentGroupResponse, error) {
	url := fmt.Sprintf("%s/DeleteAgentGroup", UserUrlPrefix)
	req := &protov2.DeleteAgentGroupRequest{}
	res := &protov2.DeleteAgentGroupResponse{}
	req.RequestId = requestId
	req.GroupName = groupName

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func GetAppliedPipelineConfigForAgentGroup(requestId []byte, groupName string) (*protov2.GetAppliedConfigsForAgentGroupResponse, error) {
	url := fmt.Sprintf("%s/GetAppliedPipelineConfigsForAgentGroup", UserUrlPrefix)
	req := &protov2.GetAppliedConfigsForAgentGroupRequest{}
	res := &protov2.GetAppliedConfigsForAgentGroupResponse{}
	req.RequestId = requestId
	req.GroupName = groupName

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}
