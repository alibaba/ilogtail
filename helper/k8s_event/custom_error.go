package k8s_event

import (
	"encoding/json"
	"regexp"
	"context"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type CustomError struct {
	Code      string
	RequestId string
	Message   string
	RawError  error
}

func (e *CustomError) CustomError() string {
	err, _ := json.Marshal(e)
	return string(err)
}

func CustomErrorNew(code string, requestId string, msg string) *CustomError {
	return &CustomError{
		Code:      code,
		RequestId: requestId,
		Message:   msg,
	}
}

type SDKError struct {
	HttpCode     int    `json:"httpCode"`
	ErrorCode    string `json:"errorCode"`
	ErrorMessage string `json:"errorMessage"`
	RequestID    string `json:"requestID"`
}

func CustomErrorFromSlsSDKError(slsSDKError error) *CustomError {
	var sdkError SDKError
	err := json.Unmarshal([]byte(slsSDKError.Error()), &sdkError)
	if err != nil {
		logger.Error(context.Background(), "unmarshal json fail : ", err.Error())
		return &CustomError{
			Code:      "SDKError",
			RequestId: "",
			Message:   slsSDKError.Error(),
			RawError:  slsSDKError,
		}
	} else {
		return &CustomError{
			Code:      sdkError.ErrorCode,
			RequestId: sdkError.RequestID,
			Message:   sdkError.ErrorMessage,
			RawError:  slsSDKError,
		}
	}
}

func CustomErrorFromPopError(popError error) *CustomError {
	compileRegex := regexp.MustCompile(`[\s\S]*ErrorCode: (.*)[\s\S]*RequestId: (.*)[\s\S]*Message: (.*)`)
	matchArr := compileRegex.FindStringSubmatch(popError.Error())
	if len(matchArr) == 0 {
		return &CustomError{
			Code:      "PopError",
			RequestId: "",
			Message:   popError.Error(),
			RawError:  popError,
		}
	}

	return &CustomError{
		Code:      matchArr[1],
		RequestId: matchArr[2],
		Message:   matchArr[3],
		RawError:  popError,
	}
}

