package monitor

import (
	"log"

	"github.com/monitor_system_go/proto"
	"github.com/monitor_system_go/worker/ebpf"
)

type NetEbpfMonitor struct {
	collector *ebpf.NetStatsCollector
	prevStats map[string]ebpf.NetStats
}

func NewNetEbpfMonitor() *NetEbpfMonitor {
	collector, err := ebpf.NewNetStatsCollector()
	if err != nil {
		log.Printf("创建eBPF网络收集器失败: %v，将使用普通网络监控器", err)
		return nil
	}

	return &NetEbpfMonitor{
		collector: collector,
		prevStats: make(map[string]ebpf.NetStats),
	}
}

func (m *NetEbpfMonitor) UpdateOnce(info *proto.MonitorInfo) {
	if m.collector == nil {
		return
	}

	// 获取当前统计数据
	currentStats, err := m.collector.GetStats()
	if err != nil {
		log.Printf("获取eBPF网络统计数据失败: %v", err)
		return
	}

	// 计算速率并更新监控信息
	for iface, stats := range currentStats {
		if prevStats, ok := m.prevStats[iface]; ok {
			// 计算速率（假设1秒间隔）
			rcvRate := int64(stats.RcvBytes - prevStats.RcvBytes)
			sndRate := int64(stats.SndBytes - prevStats.SndBytes)
			rcvPacketsRate := int64(stats.RcvPackets - prevStats.RcvPackets)
			sndPacketsRate := int64(stats.SndPackets - prevStats.SndPackets)

			info.NetInfo = append(info.NetInfo, &proto.NetInfo{
				Name:            iface,
				SendRate:        sndRate,
				RcvRate:         rcvRate,
				SendPacketsRate: sndPacketsRate,
				RcvPacketsRate:  rcvPacketsRate,
			})
		}

		// 更新历史统计数据
		m.prevStats[iface] = stats
	}

	// 重置eBPF统计数据，以便下次计算速率
	m.collector.ResetStats()
}

func (m *NetEbpfMonitor) Stop() {
	if m.collector != nil {
		m.collector.Close()
	}
}
