package handler

import (
	"config-server/common"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
	"reflect"
)

type ProtobufFunc[T any, R any] func(request *T, response *R) error

func ProtobufHandler[T any, R any](handler ProtobufFunc[T, R]) gin.HandlerFunc {
	return func(c *gin.Context) {
		var err error
		request := new(T)
		response := new(R)

		defer func() {
			responseCommon := common.GenerateCommonResponse(err)
			reflect.ValueOf(response).Elem().FieldByName("CommonResponse").Set(reflect.ValueOf(responseCommon))
			common.SuccessProtobufRes(c, response)
		}()
		err = c.ShouldBindBodyWith(request, binding.ProtoBuf)

		requestId := reflect.ValueOf(request).Elem().FieldByName("RequestId").Bytes()
		reflect.ValueOf(response).Elem().FieldByName("RequestId").Set(reflect.ValueOf(requestId))
		if requestId == nil {
			err = common.ValidateErrorWithMsg("required fields requestId could not be null")
			return
		}

		if err != nil {
			err = common.SystemError(err)
			return
		}

		err = handler(request, response)
		if err != nil {
			err = common.SystemError(err)
			return
		}
	}
}
