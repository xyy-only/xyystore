package monitor

import (
	"os"

	"github.com/monitor_system_go/proto"
)

type MetricCollector struct {
	hostname string
	monitors []Monitor
}

type Monitor interface {
	UpdateOnce(info *proto.MonitorInfo)
	Stop()
}

func NewMetricCollector() *MetricCollector {
	hostname, err := os.Hostname()
	if err != nil {
		hostname = "unknown"
	}

	monitors := []Monitor{
		NewCpuLoadMonitor(),
		NewCpuStatMonitor(),
		NewCpuSoftIrqMonitor(),
		NewMemMonitor(),
		NewDiskMonitor(),
		NewHostInfoMonitor(),
	}

	// 优先使用eBPF网络监控器
	if netEbpfMonitor := NewNetEbpfMonitor(); netEbpfMonitor != nil {
		monitors = append(monitors, netEbpfMonitor)
	} else {
		// 如果eBPF失败，使用普通网络监控器
		monitors = append(monitors, NewNetMonitor())
	}

	collector := &MetricCollector{
		hostname: hostname,
		monitors: monitors,
	}

	return collector
}

func (c *MetricCollector) CollectAll() *proto.MonitorInfo {
	info := &proto.MonitorInfo{
		Name: c.hostname,
	}

	for _, monitor := range c.monitors {
		monitor.UpdateOnce(info)
	}

	return info
}

func (c *MetricCollector) Stop() {
	for _, monitor := range c.monitors {
		monitor.Stop()
	}
}
