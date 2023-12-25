package main

import (
	"bufio"
	"context"
	"fmt"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/shirou/gopsutil/disk"
	"os"
	"strings"
	"time"
)

func main() {
	logger.Init()
	for {
		time.Sleep(time.Second * 3)
		collectDiskUsage()
		collectDiskIOCounters()
	}

}

func collectDiskIOCounters() {
	logger.Info(context.Background(), "collectDiskIOCounters", "begin")
	defer func() {
		logger.Info(context.Background(), "collectDiskIOCounters", "end")
	}()
	logger.Info(context.Background(), "collectDiskIOCounters", "IOCounters begin")
	disk.IOCounters()
	logger.Info(context.Background(), "collectDiskIOCounters", " IOCounters end")

}

func collectDiskUsage() {
	logger.Info(context.Background(), "collectDiskUsage", "begin")

	defer func() {
		logger.Info(context.Background(), "collectDiskUsage", "end")
	}()
	file, err := os.Open("/proc/1/mounts")
	if err != nil {
		fmt.Printf("%v\n", err)
	}
	defer func(file *os.File) {
		_ = file.Close()
	}(file)
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		text := scanner.Text()
		parts := strings.Fields(text)
		if len(parts) < 4 {
			return
		}
		parts[1] = strings.ReplaceAll(parts[1], "\\040", " ")
		parts[1] = strings.ReplaceAll(parts[1], "\\011", "\t")

		// wrapper with mountedpath because of using unix statfs rather than proc file system.
		logger.Info(context.Background(), "collectDiskUsage", "disk usage begin", parts[1])
		disk.Usage(parts[1])
		logger.Info(context.Background(), "collectDiskUsage", "disk usage end", parts[1])

	}
}
