package service

import (
	"time"

	"github.com/monitor_system_go/proto"
	"gorm.io/gorm"
)

type HostManager struct {
	db *gorm.DB
}

func NewHostManager(db *gorm.DB) *HostManager {
	return &HostManager{
		db: db,
	}
}

func (h *HostManager) ProcessMonitorInfo(info *proto.MonitorInfo) error {
	// 计算服务器评分
	score := h.calculateScore(info)

	// 保存性能数据
	performanceRecord := &PerformanceRecord{
		ServerName:      info.Name,
		Timestamp:       time.Now(),
		CpuPercent:      h.getAvgCpuPercent(info.CpuStat),
		MemUsedPercent:  info.MemInfo.UsedPercent,
		DiskUtilPercent: h.getAvgDiskUtil(info.DiskInfo),
		LoadAvg_1:       info.CpuLoad.LoadAvg_1,
		Score:           score,
	}

	// 保存到数据库
	if err := h.db.Create(performanceRecord).Error; err != nil {
		return err
	}

	// 检查是否有异常
	h.checkAnomalies(info, performanceRecord)

	return nil
}

func (h *HostManager) calculateScore(info *proto.MonitorInfo) float32 {
	// 简单的评分算法
	cpuScore := 100 - h.getAvgCpuPercent(info.CpuStat)
	memScore := 100 - info.MemInfo.UsedPercent
	diskScore := 100 - h.getAvgDiskUtil(info.DiskInfo)
	loadScore := 100 - info.CpuLoad.LoadAvg_1*10

	// 加权平均
	totalScore := (cpuScore*0.3 + memScore*0.3 + diskScore*0.2 + loadScore*0.2)
	if totalScore < 0 {
		totalScore = 0
	}
	if totalScore > 100 {
		totalScore = 100
	}

	return totalScore
}

func (h *HostManager) getAvgCpuPercent(cpuStats []*proto.CpuStat) float32 {
	if len(cpuStats) == 0 {
		return 0
	}

	total := float32(0)
	for _, stat := range cpuStats {
		if stat.CpuName == "cpu" {
			return stat.CpuPercent
		}
		total += stat.CpuPercent
	}

	return total / float32(len(cpuStats))
}

func (h *HostManager) getAvgDiskUtil(diskInfos []*proto.DiskInfo) float32 {
	if len(diskInfos) == 0 {
		return 0
	}

	total := float32(0)
	for _, info := range diskInfos {
		total += info.UtilPercent
	}

	return total / float32(len(diskInfos))
}

func (h *HostManager) checkAnomalies(info *proto.MonitorInfo, record *PerformanceRecord) {
	// 检查CPU异常
	if record.CpuPercent > 80 {
		anomaly := &AnomalyRecord{
			ServerName:  info.Name,
			Timestamp:   time.Now(),
			AnomalyType: "CPU_HIGH",
			Severity:    "WARNING",
			Value:       record.CpuPercent,
			Threshold:   80,
			MetricName:  "cpu_percent",
		}
		h.db.Create(anomaly)
	}

	// 检查内存异常
	if record.MemUsedPercent > 90 {
		anomaly := &AnomalyRecord{
			ServerName:  info.Name,
			Timestamp:   time.Now(),
			AnomalyType: "MEM_HIGH",
			Severity:    "WARNING",
			Value:       record.MemUsedPercent,
			Threshold:   90,
			MetricName:  "mem_used_percent",
		}
		h.db.Create(anomaly)
	}

	// 检查磁盘异常
	if record.DiskUtilPercent > 85 {
		anomaly := &AnomalyRecord{
			ServerName:  info.Name,
			Timestamp:   time.Now(),
			AnomalyType: "DISK_HIGH",
			Severity:    "WARNING",
			Value:       record.DiskUtilPercent,
			Threshold:   85,
			MetricName:  "disk_util_percent",
		}
		h.db.Create(anomaly)
	}
}
