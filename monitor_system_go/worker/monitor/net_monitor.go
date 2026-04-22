package monitor

import (
	"bufio"
	"os"
	"strconv"
	"strings"

	"github.com/monitor_system_go/proto"
)

type NetMonitor struct {
	prevStats map[string]map[string]uint64
}

func NewNetMonitor() *NetMonitor {
	return &NetMonitor{
		prevStats: make(map[string]map[string]uint64),
	}
}

func (m *NetMonitor) UpdateOnce(info *proto.MonitorInfo) {
	file, err := os.Open("/proc/net/dev")
	if err != nil {
		return
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	// 跳过前两行（标题）
	scanner.Scan()
	scanner.Scan()

	for scanner.Scan() {
		line := scanner.Text()
		parts := strings.Fields(line)
		if len(parts) >= 17 {
			interfaceName := strings.TrimSuffix(parts[0], ":")
			// 跳过回环接口
			if interfaceName == "lo" {
				continue
			}

			rcvBytes, _ := strconv.ParseUint(parts[1], 10, 64)
			rcvPackets, _ := strconv.ParseUint(parts[2], 10, 64)
			sndBytes, _ := strconv.ParseUint(parts[9], 10, 64)
			sndPackets, _ := strconv.ParseUint(parts[10], 10, 64)

			if prevStats, ok := m.prevStats[interfaceName]; ok {
				// 计算速率（假设1秒间隔）
				rcvRate := float64(rcvBytes - prevStats["rcvBytes"])
				sndRate := float64(sndBytes - prevStats["sndBytes"])
				rcvPacketsRate := float64(rcvPackets - prevStats["rcvPackets"])
				sndPacketsRate := float64(sndPackets - prevStats["sndPackets"])

				info.NetInfo = append(info.NetInfo, &proto.NetInfo{
					Name:            interfaceName,
					SendRate:        int64(sndRate),
					RcvRate:         int64(rcvRate),
					SendPacketsRate: int64(sndPacketsRate),
					RcvPacketsRate:  int64(rcvPacketsRate),
				})
			}

			// 更新统计数据
			m.prevStats[interfaceName] = map[string]uint64{
				"rcvBytes":   rcvBytes,
				"rcvPackets": rcvPackets,
				"sndBytes":   sndBytes,
				"sndPackets": sndPackets,
			}
		}
	}
}

func (m *NetMonitor) Stop() {
	// 无需特殊处理
}
