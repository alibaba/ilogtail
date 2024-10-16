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
			Status:       int32(apiError.Code),
			ErrorMessage: []byte(apiError.Message),
		}
	}
	return &proto.CommonResponse{
		Status:       int32(Failed.Code),
		ErrorMessage: []byte(err.Error()),
	}
}
