package test

import (
	"config-server/protov2"
	"fmt"
	"google.golang.org/protobuf/proto"
	"reflect"
	"testing"
)

func NewObj[T proto.Message]() {
	var ptr T
	fmt.Println(reflect.TypeOf(ptr))
	fmt.Println(reflect.New(reflect.TypeOf(ptr)).Interface())
}

func TestProto(t *testing.T) {
	//NewObj[protov2.CreateAgentGroupRequest]()
	NewObj[*protov2.CreateAgentGroupRequest]()
}
