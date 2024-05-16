package mysqlbinlog

import (
	"fmt"
	"reflect"
	"runtime"
	"strings"
	"testing"
)

func TestErr_Error(t *testing.T) {
	innerErr := fmt.Errorf("inner error")
	outerErr := &Err{
		message: "outer error",
		cause:   innerErr,
	}
	want := "outer error: inner error"
	got := outerErr.Error()
	if got != want {
		t.Errorf("Err.Error() = %q, want %q", got, want)
	}
}

func TestErr_SetLocation(t *testing.T) {
	err := &Err{}
	err.SetLocation(0)
	_, file, _, _ := runtime.Caller(0) // 本测试函数的位置
	if err.file != file {
		t.Errorf("Err.file = %q, want %q", err.file, file)
	}
	// 因为行号可能变动，这里我们只测试文件是否一致
}

func TestMysqlBinlogErrorTrace(t *testing.T) {
	innerErr := fmt.Errorf("inner error")
	got := mysqlBinlogErrorTrace(innerErr)
	want := &Err{previous: innerErr, cause: innerErr}
	if !reflect.DeepEqual(got.(*Err).cause, want.cause) || !reflect.DeepEqual(got.(*Err).previous, want.previous) {
		t.Errorf("mysqlBinlogErrorTrace() = %v, want %v", got, want)
	}
}

func TestMysqlBinlogErrorf(t *testing.T) {
	msg := "an error occurred: %v"
	arg := "connection lost"
	wantMessage := "an error occurred: connection lost"
	got := mysqlBinlogErrorf(msg, arg)
	if !strings.Contains(got.Error(), wantMessage) {
		t.Errorf("mysqlBinlogErrorf() message = %q, want contain %q", got.Error(), wantMessage)
	}
}

func TestCause(t *testing.T) {
	innerErr := fmt.Errorf("inner error")
	wrapperErr := &Err{cause: innerErr}

	got := Cause(wrapperErr)
	if got != innerErr {
		t.Errorf("Cause() = %v, want %v", got, innerErr)
	}
}
