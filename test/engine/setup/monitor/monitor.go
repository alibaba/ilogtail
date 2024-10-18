package monitor

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
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
	ctx          context.Context
	stopCh       chan bool
	isMonitoring atomic.Bool
	statistic    *Statistic
	mu           sync.Mutex
}

func StartMonitor(ctx context.Context, containerName string) (context.Context, error) {
	return monitor.Start(ctx, containerName)
}

func StopMonitor() error {
	return monitor.Stop()
}

func StopMonitorAndVerifyFinished(ctx context.Context, timeout int) (context.Context, error) {
	time.Sleep(time.Duration(timeout) * time.Second)
	cpuRawData := monitor.getCpuRawData()
	lastCpuRawData := cpuRawData[len(cpuRawData)-1]

	// Step 1: Sort the data
	sort.Float64s(cpuRawData)

	// Step 2: Calculate Q1 and Q3
	Q1 := cpuRawData[len(cpuRawData)/4]
	Q3 := cpuRawData[3*len(cpuRawData)/4]

	// Step 3: Calculate IQR
	IQR := Q3 - Q1

	// Step 4: Determine the lower bounds for outliers
	lowerBound := Q1 - 1.5*IQR

	// Step 5: Find out if the outliers exist at the tail
	if lastCpuRawData > lowerBound {
		return ctx, fmt.Errorf("Benchmark not finished, CPU usage is still high")
	}
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
			m.stopCh = make(chan bool)
			m.isMonitoring.Store(true)
			fmt.Println("Start monitoring container:", containerFullName)
			go m.monitoring(client, containerFullName)
			return ctx, nil
		}
	}
	err = fmt.Errorf("container %s not found", containerName)
	return ctx, err
}

func (m *Monitor) Stop() error {
	if m.isMonitoring.Load() {
		m.stopCh <- true
	}
	return nil
}

func (m *Monitor) monitoring(client *client.Client, containerName string) {
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
	m.statistic = NewMonitorStatistic(config.CaseName)
	for {
		select {
		case <-m.stopCh:
			m.isMonitoring.Store(false)
			bytes, _ := m.statistic.MarshalStatisticJSON()
			_ = os.WriteFile(statisticFile, bytes, 0600)
			bytes, _ = m.statistic.MarshalRecordsJSON()
			_ = os.WriteFile(recordsFile, bytes, 0600)
			return
		case <-ticker.C:
			// 获取容器信息
			containerInfo, err := client.DockerContainer(containerName, request)
			if err != nil {
				fmt.Println("Error getting container info:", err)
				return
			}
			m.mu.Lock()
			for _, stat := range containerInfo.Stats {
				m.statistic.UpdateStatistic(stat)
			}
			m.mu.Unlock()
		}
	}
}

func (m *Monitor) getCpuRawData() []float64 {
	m.mu.Lock()
	defer m.mu.Unlock()
	return m.statistic.GetCpuRawData()
}
