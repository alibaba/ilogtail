package monitor

import (
	"context"
	"fmt"
	"log"
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
	reportDir := filepath.Join(config.CaseHome, "report")
	reportDir, err := filepath.Abs(reportDir)
	if err != nil {
		log.Fatalf("Failed to get absolute path: %s", err)
	}
	if _, err = os.Stat(reportDir); os.IsNotExist(err) {
		// 文件夹不存在，创建文件夹
		err = os.MkdirAll(reportDir, 0750)
		if err != nil {
			log.Fatalf("Failed to create folder: %s", err)
		}
	}
	// new ticker
	ticker := time.NewTicker(interval * time.Second)
	defer ticker.Stop()
	// read from cadvisor per interval seconds
	request := &v1.ContainerInfoRequest{NumStats: 10}
	monitorStatistic := NewMonitorStatistic()
	for {
		select {
		case <-stopCh:
			isMonitoring.Store(false)
			var builder strings.Builder
			builder.WriteString("Metric,Value\n")
			builder.WriteString(fmt.Sprintf("%s,%f\n", "CPU Usage Max(%)", monitorStatistic.cpu.maxVal))
			builder.WriteString(fmt.Sprintf("%s,%f\n", "CPU Usage Avg(%)", monitorStatistic.cpu.avgVal))
			builder.WriteString(fmt.Sprintf("%s,%f\n", "Memory Usage Max(MB)", monitorStatistic.mem.maxVal))
			builder.WriteString(fmt.Sprintf("%s,%f\n", "Memory Usage Avg(MB)", monitorStatistic.mem.avgVal))
			err = os.WriteFile(filepath.Join(reportDir, "monitor.csv"), []byte(builder.String()), 0600)
			if err != nil {
				log.Default().Printf("Failed to write monitor result: %s", err)
			}
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
