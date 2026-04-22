package monitor

import (
	"bufio"
	"os"
	"strconv"
	"strings"

	"github.com/monitor_system_go/proto"
)

type CpuLoadMonitor struct{}

func NewCpuLoadMonitor() *CpuLoadMonitor {
	return &CpuLoadMonitor{}
}

func (m *CpuLoadMonitor) UpdateOnce(info *proto.MonitorInfo) {
	file, err := os.Open("/proc/loadavg")
	if err != nil {
		return
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	if scanner.Scan() {
		line := scanner.Text()
		parts := strings.Fields(line)
		if len(parts) >= 3 {
			load1, err1 := strconv.ParseFloat(parts[0], 32)
			load5, err2 := strconv.ParseFloat(parts[1], 32)
			load15, err3 := strconv.ParseFloat(parts[2], 32)
			if err1 == nil && err2 == nil && err3 == nil {
				info.CpuLoad = &proto.CpuLoad{
					LoadAvg_1:  float32(load1),
					LoadAvg_3:  float32(load5),
					LoadAvg_15: float32(load15),
				}
			}
		}
	}
}

func (m *CpuLoadMonitor) Stop() {
	// 无需特殊处理
}
