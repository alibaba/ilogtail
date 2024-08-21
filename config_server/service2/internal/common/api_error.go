package common

import (
	"fmt"
)

type ApiError struct {
	Code    int
	Message string
}

func (apiError ApiError) Error() string {
	return fmt.Sprintf("Code:%d,Messgae:%s", apiError.Code, apiError.Message)
}

func BaseError(code int, err error) ApiError {
	var apiErrorResult = ApiError{
		Code:    Failed.Code,
		Message: Failed.Message,
	}
	if code != 0 {
		apiErrorResult.Code = code
	}
	if err != nil {
		apiErrorResult.Message = err.Error()
	}
	return apiErrorResult
}

func ErrorWithMsg(code int, msg string) ApiError {
	var apiErrorResult = ApiError{
		Code:    Failed.Code,
		Message: Failed.Message,
	}
	if code != 0 {
		apiErrorResult.Code = code
	}
	if msg != "" {
		apiErrorResult.Message = msg
	}
	return apiErrorResult
}

func ServerErrorWithMsg(msg string, a ...any) ApiError {
	if a == nil {
		return ErrorWithMsg(Failed.Code, msg)
	}
	return ErrorWithMsg(Failed.Code, fmt.Sprintf(msg, a))
}

func ServerError() ApiError {
	return ErrorWithMsg(Failed.Code, Failed.Message)
}

func ValidateErrorWithMsg(msg string) ApiError {
	return ErrorWithMsg(ValidateFailed.Code, msg)
}

func ValidateError() ApiError {
	return ErrorWithMsg(ValidateFailed.Code, ValidateFailed.Message)
}
