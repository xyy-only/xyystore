package monitor

import (
	"bufio"
	"os"
	"strconv"
	"strings"

	"github.com/monitor_system_go/proto"
)

type CpuSoftIrqMonitor struct{}

func NewCpuSoftIrqMonitor() *CpuSoftIrqMonitor {
	return &CpuSoftIrqMonitor{}
}

func (m *CpuSoftIrqMonitor) UpdateOnce(info *proto.MonitorInfo) {
	file, err := os.Open("/proc/softirqs")
	if err != nil {
		return
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	// 跳过第一行（标题）
	scanner.Scan()
	for scanner.Scan() {
		line := scanner.Text()
		parts := strings.Fields(line)
		if len(parts) >= 11 {
			cpuName := strings.TrimSuffix(parts[0], ":")
			hi, _ := strconv.ParseInt(parts[1], 10, 64)
			timer, _ := strconv.ParseInt(parts[2], 10, 64)
			netTx, _ := strconv.ParseInt(parts[3], 10, 64)
			netRx, _ := strconv.ParseInt(parts[4], 10, 64)
			block, _ := strconv.ParseInt(parts[5], 10, 64)
			irqPoll, _ := strconv.ParseInt(parts[6], 10, 64)
			tasklet, _ := strconv.ParseInt(parts[7], 10, 64)
			sched, _ := strconv.ParseInt(parts[8], 10, 64)
			hrtimer, _ := strconv.ParseInt(parts[9], 10, 64)
			rcu, _ := strconv.ParseInt(parts[10], 10, 64)

			info.SoftIrq = append(info.SoftIrq, &proto.SoftIrq{
				Cpu:     cpuName,
				Hi:      float32(hi),
				Timer:   float32(timer),
				NetTx:   float32(netTx),
				NetRx:   float32(netRx),
				Block:   float32(block),
				IrqPoll: float32(irqPoll),
				Tasklet: float32(tasklet),
				Sched:   float32(sched),
				Hrtimer: float32(hrtimer),
				Rcu:     float32(rcu),
			})
		}
	}
}

func (m *CpuSoftIrqMonitor) Stop() {
	// 无需特殊处理
}
