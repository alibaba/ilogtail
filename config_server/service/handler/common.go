package handler

import (
	"config-server/common"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
	"log"
	"reflect"
	"strings"
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

		err = (RequestLogger(c.Request.URL.String(), handler))(request, response)
		//err = (handler)(request, response)
		if err != nil {
			err = common.SystemError(err)
			return
		}
	}
}

func RequestLogger[T any, R any](url string, handler ProtobufFunc[T, R]) ProtobufFunc[T, R] {
	return func(req *T, res *R) error {
		if strings.Contains(url, "Heart") {
			log.Printf("%s REQUEST:%+v", url, req)
		}
		err := handler(req, res)
		if err != nil {
			return common.SystemError(err)
		}
		if strings.Contains(url, "Heart") {
			log.Printf("%s RESPONSE:%+v", url, res)
		}
		return nil
	}
}
