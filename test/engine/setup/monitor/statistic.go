package monitor

import (
	"fmt"

	v1 "github.com/google/cadvisor/info/v1"
)

type Info struct {
	maxVal float64
	avgVal float64
	cnt    int64
}

func (s *Info) Add(val float64) {
	if s.cnt < 1 {
		s.maxVal = val
		s.avgVal = val
		s.cnt++
		return
	}
	if s.maxVal < val {
		s.maxVal = val
	}
	s.avgVal = float64(s.cnt)/float64(s.cnt+1)*s.avgVal + val/float64(s.cnt+1)
	s.cnt++
}

type Statistic struct {
	cpu      Info
	mem      Info
	lastStat *v1.ContainerStats
}

func NewMonitorStatistic() *Statistic {
	return &Statistic{
		cpu:      Info{},
		mem:      Info{},
		lastStat: nil,
	}
}

func (m *Statistic) UpdateStatistic(stat *v1.ContainerStats) {
	if m.lastStat != nil && !stat.Timestamp.After(m.lastStat.Timestamp) {
		return
	}
	cpuUsageRateTotal := calculateCPUUsageRate(m.lastStat, stat)
	m.cpu.Add(cpuUsageRateTotal)
	m.mem.Add(float64(stat.Memory.Usage) / 1024 / 1024)
	m.lastStat = stat
	fmt.Println("CPU Usage Rate(%):", cpuUsageRateTotal, "CPU Usage Rate Max(%):", m.cpu.maxVal, "CPU Usage Rate Avg(%):", m.cpu.avgVal, "Memory Usage Max(MB):", m.mem.maxVal, "Memory Usage Avg(MB):", m.mem.avgVal)
}

func calculateCPUUsageRate(lastStat, stat *v1.ContainerStats) float64 {
	if lastStat == nil {
		return 0
	}
	cpuUsageTotal := stat.Cpu.Usage.Total - lastStat.Cpu.Usage.Total
	cpuUsageRateTotal := float64(cpuUsageTotal) * 100 / float64(stat.Timestamp.Sub(lastStat.Timestamp).Nanoseconds())
	return cpuUsageRateTotal
}
