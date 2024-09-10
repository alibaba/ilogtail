package common

import (
	proto "config-server2/internal/protov2"
	"errors"
)

func GenerateCommonResponse(err error) *proto.CommonResponse {
	var apiError *ApiError
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
