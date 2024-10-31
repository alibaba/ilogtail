package request

import (
	"config-server/protov2"
	"fmt"
)

func ListAgents(requestId []byte, groupName string) (*protov2.ListAgentsResponse, error) {
	url := fmt.Sprintf("%s/ListAgents", UserUrlPrefix)
	req := &protov2.ListAgentsRequest{}
	res := &protov2.ListAgentsResponse{}
	req.RequestId = requestId
	req.GroupName = groupName

	err := sendProtobufRequest(url, POST, req, res)
	if err != nil {
		return nil, err
	}
	return res, nil
}
