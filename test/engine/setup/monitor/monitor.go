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
	header := "Timestamp,CPU Total(%),CPU User(%),CPU System(%),CPU Load,Memory Usage(MB)\n"
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
	var lastStat *v1.ContainerStats
	for {
		select {
		case <-stopCh:
			return
		case <-ticker.C:
			// 获取容器信息
			containerInfo, err := client.DockerContainer(containerName, request)
			if err != nil {
				fmt.Println("Error getting container info:", err)
				return
			}
			for _, stat := range containerInfo.Stats {
				if lastStat == nil {
					lastStat = stat
					continue
				}
				timestamp := stat.Timestamp
				if !timestamp.After(lastStat.Timestamp) {
					continue
				}
				// 计算cpu使用率
				cpuUsageRateTotal, cpuUsageRateUser, cpuUsageRateSys := calculateCpuUsageRate(lastStat, stat)
				// 写入文件
				_, err := file.WriteString(fmt.Sprintf("%s,%f,%f,%f,%d,%d\n",
					timestamp.Format("2006-01-02 15:04:05"),
					cpuUsageRateTotal,
					cpuUsageRateUser,
					cpuUsageRateSys,
					stat.Cpu.LoadAverage,
					stat.Memory.Usage/1024/1024,
				))
				if err != nil {
					fmt.Println("Error writing file:", err)
					return
				}
				lastStat = stat
			}
		}
	}
}

func calculateCpuUsageRate(lastStat, stat *v1.ContainerStats) (float64, float64, float64) {
	if lastStat == nil {
		return 0, 0, 0
	}
	cpuUsageTotal := stat.Cpu.Usage.Total - lastStat.Cpu.Usage.Total
	cpuUsageUser := stat.Cpu.Usage.User - lastStat.Cpu.Usage.User
	cpuUsageSys := stat.Cpu.Usage.System - lastStat.Cpu.Usage.System
	cpuUsageRateTotal := float64(cpuUsageTotal) / float64(stat.Timestamp.Sub(lastStat.Timestamp).Nanoseconds()) * 100
	cpuUsageRateUser := float64(cpuUsageUser) / float64(stat.Timestamp.Sub(lastStat.Timestamp).Nanoseconds()) * 100
	cpuUsageRateSys := float64(cpuUsageSys) / float64(stat.Timestamp.Sub(lastStat.Timestamp).Nanoseconds()) * 100
	return cpuUsageRateTotal, cpuUsageRateUser, cpuUsageRateSys
}
