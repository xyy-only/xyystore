package monitor

import (
	"net"

	"github.com/monitor_system_go/proto"
)

type HostInfoMonitor struct{}

func NewHostInfoMonitor() *HostInfoMonitor {
	return &HostInfoMonitor{}
}

func (m *HostInfoMonitor) UpdateOnce(info *proto.MonitorInfo) {
	// 获取主机名
	hostname := info.Name

	// 获取IP地址
	ipAddress := ""
	addrs, err := net.InterfaceAddrs()
	if err == nil {
		for _, addr := range addrs {
			if ipnet, ok := addr.(*net.IPNet); ok && !ipnet.IP.IsLoopback() {
				if ipnet.IP.To4() != nil {
					ipAddress = ipnet.IP.String()
					break
				}
			}
		}
	}

	info.HostInfo = &proto.HostInfo{
		Hostname:  hostname,
		IpAddress: ipAddress,
	}
}

func (m *HostInfoMonitor) Stop() {
	// 无需特殊处理
}
