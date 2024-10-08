package common

import (
	"log"
	"runtime"
)

func PrintLog(code int, msg string, skip int) {
	_, file, line, _ := runtime.Caller(skip)
	log.Printf("[ERROR]: [%s:%d] Code:%d,Message:%s\n", file, line, code, msg)
}
