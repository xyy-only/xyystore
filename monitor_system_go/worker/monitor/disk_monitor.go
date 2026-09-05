package monitor

import (
	"bufio"
	"os"
	"strconv"
	"strings"

	"github.com/monitor_system_go/proto"
)

type DiskMonitor struct {
	prevStats map[string]map[string]uint64
}

func NewDiskMonitor() *DiskMonitor {
	return &DiskMonitor{
		prevStats: make(map[string]map[string]uint64),
	}
}

func (m *DiskMonitor) UpdateOnce(info *proto.MonitorInfo) {
	file, err := os.Open("/proc/diskstats")
	if err != nil {
		return
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		parts := strings.Fields(line)
		if len(parts) >= 14 {
			diskName := parts[2]
			// 只处理物理磁盘，跳过分区
			if len(diskName) > 3 && diskName[0:3] == "sd" && len(diskName) == 3 {
				readIOs, _ := strconv.ParseUint(parts[3], 10, 64)
				readBytes, _ := strconv.ParseUint(parts[5], 10, 64)
				writeIOs, _ := strconv.ParseUint(parts[7], 10, 64)
				writeBytes, _ := strconv.ParseUint(parts[9], 10, 64)
				ioTime, _ := strconv.ParseUint(parts[12], 10, 64)

				if prevStats, ok := m.prevStats[diskName]; ok {
					// 计算速率（假设1秒间隔）
					readBytesPerSec := float64(readBytes - prevStats["readBytes"])
					writeBytesPerSec := float64(writeBytes - prevStats["writeBytes"])
					readIOPS := float64(readIOs - prevStats["readIOs"])
					writeIOPS := float64(writeIOs - prevStats["writeIOs"])
					utilPercent := float64(ioTime-prevStats["ioTime"]) / 10.0

					info.DiskInfo = append(info.DiskInfo, &proto.DiskInfo{
						Name:             diskName,
						ReadBytesPerSec:  float32(readBytesPerSec),
						WriteBytesPerSec: float32(writeBytesPerSec),
						ReadIops:         float32(readIOPS),
						WriteIops:        float32(writeIOPS),
						UtilPercent:      float32(utilPercent),
					})
				}

				// 更新统计数据
				m.prevStats[diskName] = map[string]uint64{
					"readBytes":  readBytes,
					"readIOs":    readIOs,
					"writeBytes": writeBytes,
					"writeIOs":   writeIOs,
					"ioTime":     ioTime,
				}
			}
		}
	}
}

func (m *DiskMonitor) Stop() {
	// 无需特殊处理
}
