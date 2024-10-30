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
	Accept     = HttpStatus{200, "Accept"}
	BadRequest = HttpStatus{400, "BadRequest"}

	AgentAlreadyExist = HttpStatus{400, "AgentAlreadyExist"}
	AgentNotExist     = HttpStatus{404, "AgentNotExist"}

	ConfigAlreadyExist = HttpStatus{400, "ConfigAlreadyExist"}
	ConfigNotExist     = HttpStatus{404, "ConfigNotExist"}

	AgentGroupAlreadyExist = HttpStatus{400, "AgentGroupAlreadyExist"}
	AgentGroupNotExist     = HttpStatus{404, "AgentGroupNotExist"}

	InvalidParameter = HttpStatus{400, "InvalidParameter"}
	RequestTimeout   = HttpStatus{500, "RequestTimeout"}
	ServerBusy       = HttpStatus{503, "ServerBusy"}
)

func ErrorProtobufRes(c *gin.Context, res any, err error) {
	c.ProtoBuf(ServerBusy.Code, res)
}

func SuccessProtobufRes(c *gin.Context, res any) {
	c.ProtoBuf(Accept.Code, res)
}

func ProtobufRes(c *gin.Context, err error, res any) {
	var apiError *ApiError
	if err == nil {
		c.ProtoBuf(Accept.Code, res)
		return
	}
	if errors.As(err, &apiError) {
		c.ProtoBuf(apiError.Status.Code, res)
		return
	}
	c.ProtoBuf(ServerBusy.Code, res)
}
