package common

import (
	"context"
	"fmt"
	"sync"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/test/config"
	"github.com/melbahja/goph"
)

var sshClient *goph.Client
var initSSHClientOnce sync.Once

func SSHExec(command string) ([]byte, error) {
	initSSHClientOnce.Do(func() {
		auth, err := goph.Key(config.TestConfig.SSHPrivateKeyPath, "")
		if err != nil {
			logger.Errorf(context.TODO(), "SSHExec", "error in find ssh key: %v", err)
			return
		}
		client, err := goph.New(config.TestConfig.SSHUsername, config.TestConfig.SSHIP, auth)
		if err != nil {
			logger.Errorf(context.TODO(), "SSHExec", "error in create ssh client: %v", err)
			return
		}
		sshClient = client
	})

	if sshClient == nil {
		return nil, fmt.Errorf("ssh client init failed")
	}
	return sshClient.Run(command)
}
