package k8smeta

import (
	"fmt"
	"os"
	"testing"
)

func TestMetaManager(t *testing.T) {
	os.Setenv("KUBERNETES_METADATA_PORT", "9999")
	instance := GetMetaManagerInstance()
	err := instance.Init("/Users/runqi.lin/workspace/one-agent/new/kubeconfig-demo-chengdu")
	if err != nil {
		fmt.Println("init err")
		return
	}
	instance.Run()
}
