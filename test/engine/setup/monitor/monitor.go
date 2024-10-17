package monitor

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync/atomic"
	"time"

	"github.com/google/cadvisor/client"
	v1 "github.com/google/cadvisor/info/v1"

	"github.com/alibaba/ilogtail/test/config"
)

const (
	cadvisorURL = "http://localhost:8080/"
	interval    = 3
)

var stopCh chan bool
var isMonitoring atomic.Bool

func StartMonitor(ctx context.Context, containerName string) (context.Context, error) {
	// connect to cadvisor
	client, err := client.NewClient("http://localhost:8080/")
	if err != nil {
		return ctx, err
	}
	// 获取所有容器信息
	allContainers, err := client.AllDockerContainers(&v1.ContainerInfoRequest{NumStats: 10})
	if err != nil {
		fmt.Println("Error getting all containers info:", err)
		return ctx, err
	}
	for _, container := range allContainers {
		containerFullName := container.Aliases[0]
		if strings.Contains(containerFullName, containerName) {
			stopCh = make(chan bool)
			isMonitoring.Store(true)
			fmt.Println("Start monitoring container:", containerFullName)
			go monitoring(client, containerFullName)
			return ctx, nil
		}
	}
	err = fmt.Errorf("container %s not found", containerName)
	return ctx, err
}

func StopMonitor() error {
	if isMonitoring.Load() {
		stopCh <- true
	}
	return nil
}

func monitoring(client *client.Client, containerName string) {
	// create csv file
	root, _ := filepath.Abs(".")
	reportDir := root + "/report/"
	statisticFile := reportDir + config.CaseName + "_statistic.json"
	recordsFile := reportDir + config.CaseName + "_records.json"
	// new ticker
	ticker := time.NewTicker(interval * time.Second)
	defer ticker.Stop()
	// read from cadvisor per interval seconds
	request := &v1.ContainerInfoRequest{NumStats: 10}
	monitorStatistic := NewMonitorStatistic(config.CaseName)
	for {
		select {
		case <-stopCh:
			isMonitoring.Store(false)
			bytes, _ := monitorStatistic.MarshalStatisticJSON()
			_ = os.WriteFile(statisticFile, bytes, 0600)
			bytes, _ = monitorStatistic.MarshalRecordsJSON()
			_ = os.WriteFile(recordsFile, bytes, 0600)
			return
		case <-ticker.C:
			// 获取容器信息
			containerInfo, err := client.DockerContainer(containerName, request)
			if err != nil {
				fmt.Println("Error getting container info:", err)
				return
			}
			for _, stat := range containerInfo.Stats {
				monitorStatistic.UpdateStatistic(stat)
			}
		}
	}
}
