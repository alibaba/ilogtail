package monitor

import (
	"context"
	"fmt"
	"log"
	"os"
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
	stopCh = make(chan bool)
	isMonitoring.Store(true)
	// connect to cadvisor
	client, err := client.NewClient("http://localhost:8080/")
	if err != nil {
		return ctx, err
	}
	// 获取机器信息
	_, err = client.MachineInfo()
	if err != nil {
		// 处理错误
		return ctx, err
	}
	// fmt.Println("Machine Info:", machineInfo)
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
	reportDir := config.CaseHome + "/report/"
	if _, err := os.Stat(reportDir); os.IsNotExist(err) {
		// 文件夹不存在，创建文件夹
		err := os.MkdirAll(reportDir, 0755) // 使用适当的权限
		if err != nil {
			log.Fatalf("Failed to create folder: %s", err)
		}
	}
	file, err := os.OpenFile(reportDir+"performance.csv", os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0644)
	if err != nil {
		fmt.Println("Error creating file:", err)
		return
	}
	defer file.Close()
	header := "CPU Usage Max(%),CPU Usage Avg(%), Memory Usage Max(MB), Memory Usage Avg(MB)\n"
	_, err = file.WriteString(header)
	if err != nil {
		fmt.Println("Error writring file header:", err)
		return
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
			fmt.Fprintln(file, monitorStatistic.cpu.maxVal, ",", monitorStatistic.cpu.avgVal, ",", monitorStatistic.mem.maxVal, ",", monitorStatistic.mem.avgVal)
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
