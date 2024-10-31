package common

import (
	"errors"
	"fmt"
)

type ApiError struct {
	Status  HttpStatus
	Message string
}

func (apiError ApiError) Error() string {
	return fmt.Sprintf("Code:%d,Messgae:%s", apiError.Status.Code, apiError.Message)
}

func ErrorWithMsg(status HttpStatus, msg string) *ApiError {
	var apiErrorResult = ApiError{
		Status:  ServerBusy,
		Message: ServerBusy.Message,
	}
	if status.Code != 0 {
		apiErrorResult.Status = status
	}
	if msg != "" {
		apiErrorResult.Message = msg
	}
	PrintLog(apiErrorResult.Status.Code, apiErrorResult.Message, 3)
	return &apiErrorResult
}

//-----Server-----
// include config、agent、agent_group already exists or not exists

func ServerErrorWithMsg(status HttpStatus, msg string, a ...any) *ApiError {
	if a == nil || len(a) == 0 {
		return ErrorWithMsg(status, msg)
	}
	return ErrorWithMsg(status, fmt.Sprintf(msg, a...))
}

func ServerError(msg string, a ...any) *ApiError {
	return ServerErrorWithMsg(ServerBusy, msg, a...)
}

//-----InvalidParameter-----

func ValidateErrorWithMsg(msg string) *ApiError {
	return ErrorWithMsg(InvalidParameter, msg)
}

func ValidateError() *ApiError {
	return ErrorWithMsg(InvalidParameter, InvalidParameter.Message)
}

//包裹异常，并用来输出堆栈日志
//这里切记不要返回APIError,否则会出现问题（因为(ApiError)(nil)!=nil）

func SystemError(err error) error {
	if err == nil {
		return nil
	}

	var apiError *ApiError
	if errors.As(err, &apiError) {
		return ErrorWithMsg(apiError.Status, apiError.Message)
	}

	return ErrorWithMsg(ServerBusy, err.Error())
}
