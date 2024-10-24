package common

import (
	"github.com/gin-gonic/gin"
)

type HttpStatus struct {
	Code    int
	Message string
}

var (
	Success        = HttpStatus{Code: 200, Message: "success"}
	Failed         = HttpStatus{Code: 500, Message: "failed"}
	SystemFailed   = HttpStatus{Code: 505, Message: "systemFailed"}
	ValidateFailed = HttpStatus{Code: 404, Message: "parameter is not"}
)

func ErrorProtobufRes(c *gin.Context, res any, err error) {
	c.ProtoBuf(Success.Code, res)
}

func SuccessProtobufRes(c *gin.Context, res any) {
	c.ProtoBuf(Success.Code, res)
}
