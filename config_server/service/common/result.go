package common

import (
	proto "config-server/protov2"
	"errors"
)

func GenerateCommonResponse(err error) *proto.CommonResponse {
	var apiError *ApiError
	if err == nil {
		return &proto.CommonResponse{
			Status:       0,
			ErrorMessage: nil,
		}
	}
	if errors.As(err, &apiError) {
		return &proto.CommonResponse{
			Status:       int32(apiError.Status.Code),
			ErrorMessage: []byte(apiError.Status.Message),
		}
	}
	return &proto.CommonResponse{
		Status:       int32(ServerBusy.Code),
		ErrorMessage: []byte(err.Error()),
	}
}
