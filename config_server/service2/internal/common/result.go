package common

import (
	proto "config-server2/internal/common/protov2"
	"errors"
)

func GenerateServerErrorResponse(err error) *proto.ServerErrorResponse {
	var apiError = &ApiError{}
	if errors.As(err, apiError) {
		return &proto.ServerErrorResponse{
			ErrorCode:    int32(apiError.Code),
			ErrorMessage: apiError.Message,
		}
	}
	return &proto.ServerErrorResponse{
		ErrorCode:    int32(Failed.Code),
		ErrorMessage: err.Error(),
	}
}
