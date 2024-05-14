package helper

import (
	"context"
	"time"

	docker "github.com/docker/docker/client"
)

func CreateDockerClient(opt ...docker.Opt) (client *docker.Client, err error) {
	opt = append(opt, docker.FromEnv)
	client, err = docker.NewClientWithOpts(opt...)
	if err != nil {
		return nil, err
	}
	// add dockerClient connectivity tests
	pingCtx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()
	ping, err := client.Ping(pingCtx)
	if err != nil {
		return nil, err
	}
	client.NegotiateAPIVersionPing(ping)
	return
}
