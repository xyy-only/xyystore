package monitor

import (
	"bufio"
	"os"
	"strconv"
	"strings"

	"github.com/monitor_system_go/proto"
)

type MemMonitor struct{}

func NewMemMonitor() *MemMonitor {
	return &MemMonitor{}
}

func (m *MemMonitor) UpdateOnce(info *proto.MonitorInfo) {
	file, err := os.Open("/proc/meminfo")
	if err != nil {
		return
	}
	defer file.Close()

	memInfo := &proto.MemInfo{}
	memMap := make(map[string]float32)

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		parts := strings.Fields(line)
		if len(parts) >= 2 {
			key := strings.TrimSuffix(parts[0], ":")
			val, err := strconv.ParseFloat(parts[1], 32)
			if err != nil {
				continue
			}
			// 转换为GB
			if strings.HasSuffix(parts[1], "kB") {
				val /= (1024 * 1024)
			}
			memMap[key] = float32(val)
		}
	}

	// 填充内存信息
	if total, ok := memMap["MemTotal"]; ok {
		memInfo.Total = total
	}
	if free, ok := memMap["MemFree"]; ok {
		memInfo.Free = free
	}
	if avail, ok := memMap["MemAvailable"]; ok {
		memInfo.Avail = avail
		// 计算使用率
		if total, ok := memMap["MemTotal"]; ok && total > 0 {
			memInfo.UsedPercent = (1 - avail/total) * 100
		}
	}
	if buffers, ok := memMap["Buffers"]; ok {
		memInfo.Buffers = buffers
	}
	if cached, ok := memMap["Cached"]; ok {
		memInfo.Cached = cached
	}
	if swapCached, ok := memMap["SwapCached"]; ok {
		memInfo.SwapCached = swapCached
	}
	if active, ok := memMap["Active"]; ok {
		memInfo.Active = active
	}
	if inactive, ok := memMap["Inactive"]; ok {
		memInfo.Inactive = inactive
	}

	info.MemInfo = memInfo
}

func (m *MemMonitor) Stop() {
	// 无需特殊处理
}
