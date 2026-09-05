package monitor

import (
	"bufio"
	"os"
	"strconv"
	"strings"

	"github.com/monitor_system_go/proto"
)

type CpuStatMonitor struct {
	prevStats map[string][]uint64
}

func NewCpuStatMonitor() *CpuStatMonitor {
	return &CpuStatMonitor{
		prevStats: make(map[string][]uint64),
	}
}

func (m *CpuStatMonitor) UpdateOnce(info *proto.MonitorInfo) {
	file, err := os.Open("/proc/stat")
	if err != nil {
		return
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		if strings.HasPrefix(line, "cpu") {
			parts := strings.Fields(line)
			if len(parts) >= 8 {
				cpuName := parts[0]
				stats := make([]uint64, 8)
				for i := 1; i < 9; i++ {
					val, err := strconv.ParseUint(parts[i], 10, 64)
					if err != nil {
						continue
					}
					stats[i-1] = val
				}

				if prevStats, ok := m.prevStats[cpuName]; ok {
					total := uint64(0)
					prevTotal := uint64(0)
					for i := 0; i < 8; i++ {
						total += stats[i]
						prevTotal += prevStats[i]
					}

					totalDiff := total - prevTotal
					if totalDiff > 0 {
						usr := float32(stats[0]-prevStats[0]) / float32(totalDiff) * 100
						system := float32(stats[2]-prevStats[2]) / float32(totalDiff) * 100
						nice := float32(stats[1]-prevStats[1]) / float32(totalDiff) * 100
						idle := float32(stats[3]-prevStats[3]) / float32(totalDiff) * 100
						ioWait := float32(stats[4]-prevStats[4]) / float32(totalDiff) * 100
						irq := float32(stats[5]-prevStats[5]) / float32(totalDiff) * 100
						softIrq := float32(stats[6]-prevStats[6]) / float32(totalDiff) * 100
						cpuPercent := 100 - idle

						info.CpuStat = append(info.CpuStat, &proto.CpuStat{
							CpuName:        cpuName,
							CpuPercent:     cpuPercent,
							UsrPercent:     usr,
							SystemPercent:  system,
							NicePercent:    nice,
							IdlePercent:    idle,
							IoWaitPercent:  ioWait,
							IrqPercent:     irq,
							SoftIrqPercent: softIrq,
						})
					}
				}
				m.prevStats[cpuName] = stats
			}
		}
	}
}

func (m *CpuStatMonitor) Stop() {
	// 无需特殊处理
}
