package setup

import (
	"context"
	"fmt"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/test/config"
	"github.com/melbahja/goph"
	"golang.org/x/crypto/ssh"
)

type HostEnv struct {
	sshClient *goph.Client
}

func NewHostEnv() *HostEnv {
	env := &HostEnv{}
	env.initSSHClient()
	return env
}

func (h *HostEnv) Exec(command string) error {
	if h.sshClient == nil {
		return fmt.Errorf("ssh client init failed")
	}
	fmt.Println(command)
	_, err := h.sshClient.Run(command)
	return err
}

func (h *HostEnv) initSSHClient() {
	client, err := goph.NewConn(&goph.Config{
		User:     config.TestConfig.SSHUsername,
		Addr:     config.TestConfig.SSHIP,
		Port:     22,
		Auth:     goph.Password(config.TestConfig.SSHPassword),
		Callback: ssh.InsecureIgnoreHostKey(),
	})
	if err != nil {
		logger.Errorf(context.TODO(), "SSHExec", "error in create ssh client: %v", err)
		return
	}
	h.sshClient = client
}
