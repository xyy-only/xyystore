package model

import (
	"time"
)

// PerformanceRecord 性能数据记录
type PerformanceRecord struct {
	ID                  uint      `gorm:"primaryKey"`
	ServerName          string    `gorm:"index"`
	Timestamp           time.Time `gorm:"index"`
	CpuPercent          float32
	UsrPercent          float32
	SystemPercent       float32
	NicePercent         float32
	IdlePercent         float32
	IoWaitPercent       float32
	IrqPercent          float32
	SoftIrqPercent      float32
	LoadAvg_1           float32
	LoadAvg_3           float32
	LoadAvg_15          float32
	MemUsedPercent      float32
	MemTotal            float32
	MemFree             float32
	MemAvail            float32
	DiskUtilPercent     float32
	SendRate            float32
	RcvRate             float32
	Score               float32
	CpuPercentRate      float32
	UsrPercentRate      float32
	SystemPercentRate   float32
	IoWaitPercentRate   float32
	LoadAvg_1Rate       float32
	LoadAvg_3Rate       float32
	LoadAvg_15Rate      float32
	MemUsedPercentRate  float32
	DiskUtilPercentRate float32
	SendRateRate        float32
	RcvRateRate         float32
	CreatedAt           time.Time
	UpdatedAt           time.Time
}

// AnomalyRecord 异常记录
type AnomalyRecord struct {
	ID          uint      `gorm:"primaryKey"`
	ServerName  string    `gorm:"index"`
	Timestamp   time.Time `gorm:"index"`
	AnomalyType string
	Severity    string
	Value       float32
	Threshold   float32
	MetricName  string
	CreatedAt   time.Time
	UpdatedAt   time.Time
}

// ServerScoreSummary 服务器评分摘要
type ServerScoreSummary struct {
	ServerName      string
	Score           float32
	LastUpdate      time.Time
	Status          string
	CpuPercent      float32
	MemUsedPercent  float32
	DiskUtilPercent float32
	LoadAvg_1       float32
}
