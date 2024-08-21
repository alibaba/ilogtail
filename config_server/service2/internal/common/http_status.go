package common

import (
	"errors"
	"github.com/gin-gonic/gin"
)

type HttpStatus struct {
	Code    int
	Message string
}

var (
	Success        = HttpStatus{Code: 200, Message: "success"}
	Failed         = HttpStatus{Code: 500, Message: "failed"}
	ValidateFailed = HttpStatus{Code: 404, Message: "parameter is not"}
)

func ErrorProtobufRes(c *gin.Context, res any, err error) {
	var apiError = &ApiError{}
	if errors.As(err, apiError) {
		c.ProtoBuf(apiError.Code, res)
		return
	}
	c.ProtoBuf(Failed.Code, res)
}

func SuccessProtobufRes(c *gin.Context, res any) {
	c.ProtoBuf(Success.Code, res)
}
