package monitor

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"sort"
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

var monitor Monitor

type Monitor struct {
	isMonitoring atomic.Bool
	statistic    *Statistic
	stopCh       chan struct{}
}

func StartMonitor(ctx context.Context, containerName string) (context.Context, error) {
	return monitor.Start(ctx, containerName)
}

func WaitMonitorUntilProcessingFinished(ctx context.Context) (context.Context, error) {
	<-monitor.Done()
	return ctx, nil
}

func (m *Monitor) Start(ctx context.Context, containerName string) (context.Context, error) {
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
			m.isMonitoring.Store(true)
			m.statistic = NewMonitorStatistic(config.CaseName)
			m.stopCh = make(chan struct{})
			fmt.Println("Start monitoring container:", containerFullName)
			go m.monitoring(client, containerFullName)
			return ctx, nil
		}
	}
	err = fmt.Errorf("container %s not found", containerName)
	return ctx, err
}

func (m *Monitor) monitoring(client *client.Client, containerName string) {
	// create csv file
	root, _ := filepath.Abs(".")
	reportDir := root + "/report/"
	statisticFile := reportDir + config.CaseName + "_statistic.json"
	recordsFile := reportDir + config.CaseName + "_records.json"
	// calculate low threshold after 60 seconds
	timer := time.NewTimer(60 * time.Second)
	lowThreshold := 0.0
	// read from cadvisor per interval seconds
	ticker := time.NewTicker(interval * time.Second)
	defer ticker.Stop()
	request := &v1.ContainerInfoRequest{NumStats: 10}
	for {
		select {
		case <-timer.C:
			// 计算CPU使用率的下阈值
			cpuRawData := make([]float64, len(m.statistic.GetCPURawData()))
			copy(cpuRawData, m.statistic.GetCPURawData())
			sort.Float64s(cpuRawData)
			Q1 := cpuRawData[len(cpuRawData)/4]
			Q3 := cpuRawData[3*len(cpuRawData)/4]
			IQR := Q3 - Q1
			lowThreshold = Q1 - 1.5*IQR
			fmt.Println("Low threshold of CPU usage rate(%):", lowThreshold)
		case <-ticker.C:
			// 获取容器信息
			containerInfo, err := client.DockerContainer(containerName, request)
			if err != nil {
				fmt.Println("Error getting container info:", err)
				return
			}
			for _, stat := range containerInfo.Stats {
				m.statistic.UpdateStatistic(stat)
			}
			cpuRawData := m.statistic.GetCPURawData()
			if (cpuRawData[len(cpuRawData)-1] < lowThreshold) && (lowThreshold > 0) {
				m.isMonitoring.Store(false)
				m.stopCh <- struct{}{}
				bytes, _ := m.statistic.MarshalStatisticJSON()
				_ = os.WriteFile(statisticFile, bytes, 0600)
				bytes, _ = m.statistic.MarshalRecordsJSON()
				_ = os.WriteFile(recordsFile, bytes, 0600)
				return
			}
		}
	}
}

func (m *Monitor) Done() <-chan struct{} {
	return m.stopCh
}
