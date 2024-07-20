package monitor

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
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
	// 获取机器信息
	request := &v1.ContainerInfoRequest{NumStats: 10}
	_, err = client.DockerContainer(containerName, request)
	if err != nil {
		fmt.Println("Error getting container info:", err)
		return ctx, err
	}
	stopCh = make(chan bool)
	isMonitoring.Store(true)
	fmt.Println("Start monitoring container:", containerName)
	go monitoring(client, containerName)
	return ctx, nil
}

func StopMonitor(ctx context.Context) (context.Context, error) {
	if isMonitoring.Load() {
		stopCh <- true
	}
	return ctx, nil
}

func monitoring(client *client.Client, containerName string) {
	// create csv file
	root, _ := filepath.Abs(".")
	reportDir := root + "/report/"
	benchmarkFile := reportDir + config.CaseName + "_benchmark.json"
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
			bytes, _ := monitorStatistic.MarshalJSON()
			_ = os.WriteFile(benchmarkFile, bytes, 0600)
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
