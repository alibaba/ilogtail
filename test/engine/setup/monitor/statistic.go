package monitor

import (
	"encoding/json"
	"fmt"

	v1 "github.com/google/cadvisor/info/v1"
)

type Info struct {
	maxVal float64
	avgVal float64
	cnt    int64
	values []float64
}

func (s *Info) Add(val float64) {
	s.values = append(s.values, val)
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
	name     string
	cpu      Info
	mem      Info
	lastStat *v1.ContainerStats
}

func NewMonitorStatistic(name string) *Statistic {
	return &Statistic{
		name:     name,
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
	// fmt.Println("CPU Usage Rate(%):", cpuUsageRateTotal, "CPU Usage Rate Max(%):", m.cpu.maxVal, "CPU Usage Rate Avg(%):", m.cpu.avgVal, "Memory Usage Max(MB):", m.mem.maxVal, "Memory Usage Avg(MB):", m.mem.avgVal)
}

func calculateCPUUsageRate(lastStat, stat *v1.ContainerStats) float64 {
	if lastStat == nil {
		return 0
	}
	cpuUsageTotal := stat.Cpu.Usage.Total - lastStat.Cpu.Usage.Total
	cpuUsageRateTotal := float64(cpuUsageTotal) * 100 / float64(stat.Timestamp.Sub(lastStat.Timestamp).Nanoseconds())
	return cpuUsageRateTotal
}

type StatisticItem struct {
	Name  string  `json:"name"`
	Value float64 `json:"value"`
	Unit  string  `json:"unit"`
}

func (m *Statistic) MarshalStatisticJSON() ([]byte, error) {
	items := []StatisticItem{
		{"CPU_Usage_Max - " + m.name, m.cpu.maxVal, "%"},
		{"CPU_Usage_Avg - " + m.name, m.cpu.avgVal, "%"},
		{"Memory_Usage_Max - " + m.name, m.mem.maxVal, "MB"},
		{"Memory_Usage_Avg - " + m.name, m.mem.avgVal, "MB"},
	}

	// Serialize the slice to JSON
	jsonData, err := json.MarshalIndent(items, "", "    ")
	if err != nil {
		fmt.Println("Error serializing statistics:", err)
		return nil, err
	}

	// Output the JSON string
	return jsonData, nil
}

type RecordsItem struct {
	Name    string    `json:"name"`
	Records []float64 `json:"records"`
	Unit    string    `json:"unit"`
}

func (m *Statistic) MarshalRecordsJSON() ([]byte, error) {
	items := []RecordsItem{
		{"CPU_Usage - " + m.name, m.cpu.values, "%"},
		{"Memory_Usage - " + m.name, m.mem.values, "MB"},
	}

	// Serialize the slice to JSON
	jsonData, err := json.MarshalIndent(items, "", "    ")
	if err != nil {
		fmt.Println("Error serializing statistics:", err)
		return nil, err
	}

	// Output the JSON string
	return jsonData, nil
}
