package monitor

import (
	"fmt"

	v1 "github.com/google/cadvisor/info/v1"
)

type Statistic struct {
	maxVal float64
	avgVal float64
	cnt    int64
}

func (s *Statistic) Add(val float64) {
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

type MonitorStatistic struct {
	cpu      Statistic
	mem      Statistic
	lastStat *v1.ContainerStats
}

func NewMonitorStatistic() *MonitorStatistic {
	return &MonitorStatistic{
		cpu:      Statistic{},
		mem:      Statistic{},
		lastStat: nil,
	}
}

func (m *MonitorStatistic) UpdateStatistic(stat *v1.ContainerStats) {
	if m.lastStat != nil && !stat.Timestamp.After(m.lastStat.Timestamp) {
		return
	}
	cpuUsageRateTotal := calculateCpuUsageRate(m.lastStat, stat)
	m.cpu.Add(cpuUsageRateTotal)
	m.mem.Add(float64(stat.Memory.Usage) / 1024 / 1024)
	m.lastStat = stat
	fmt.Println("CPU Usage Rate(%):", cpuUsageRateTotal, "CPU Usage Rate Max(%):", m.cpu.maxVal, "CPU Usage Rate Avg(%):", m.cpu.avgVal, "Memory Usage Max(MB):", m.mem.maxVal, "Memory Usage Avg(MB):", m.mem.avgVal)
}

func calculateCpuUsageRate(lastStat, stat *v1.ContainerStats) float64 {
	if lastStat == nil {
		return 0
	}
	cpuUsageTotal := stat.Cpu.Usage.Total - lastStat.Cpu.Usage.Total
	cpuUsageRateTotal := float64(cpuUsageTotal) * 100 / float64(stat.Timestamp.Sub(lastStat.Timestamp).Nanoseconds())
	return cpuUsageRateTotal
}
