package common

import (
	"context"
	"fmt"
	"sync"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/test/config"
	"github.com/melbahja/goph"
	"golang.org/x/crypto/ssh"
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
		client, err := goph.NewConn(&goph.Config{
			User:     config.TestConfig.SSHUsername,
			Addr:     config.TestConfig.SSHIP,
			Port:     22,
			Auth:     auth,
			Callback: ssh.InsecureIgnoreHostKey(),
		})
		if err != nil {
			logger.Errorf(context.TODO(), "SSHExec", "error in create ssh client: %v", err)
			return
		}
		sshClient = client
	})

	if sshClient == nil {
		return nil, fmt.Errorf("ssh client init failed")
	}
	fmt.Println(command)
	return sshClient.Run(command)
}
