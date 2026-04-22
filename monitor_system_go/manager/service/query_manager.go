package service

import (
	"time"

	"github.com/monitor_system_go/proto"
	"gorm.io/gorm"
)

type QueryManager struct {
	db *gorm.DB
}

func NewQueryManager(db *gorm.DB) *QueryManager {
	return &QueryManager{
		db: db,
	}
}

func (q *QueryManager) QueryPerformance(req *proto.QueryPerformanceRequest) (*proto.QueryPerformanceResponse, error) {
	var records []PerformanceRecord
	var totalCount int64

	// 构建查询
	query := q.db.Model(&PerformanceRecord{})
	if req.ServerName != "" {
		query = query.Where("server_name = ?", req.ServerName)
	}
	if req.TimeRange != nil {
		query = query.Where("timestamp >= ? AND timestamp <= ?", req.TimeRange.StartTime.AsTime(), req.TimeRange.EndTime.AsTime())
	}

	// 计算总数
	query.Count(&totalCount)

	// 分页
	page := int(req.Pagination.Page)
	pageSize := int(req.Pagination.PageSize)
	offset := (page - 1) * pageSize

	// 执行查询
	query.Order("timestamp DESC").Offset(offset).Limit(pageSize).Find(&records)

	// 转换为proto响应
	response := &proto.QueryPerformanceResponse{
		TotalCount: int32(totalCount),
		Page:       req.Pagination.Page,
		PageSize:   req.Pagination.PageSize,
	}

	for _, record := range records {
		response.Records = append(response.Records, &proto.PerformanceRecord{
			ServerName:       record.ServerName,
			Timestamp:        record.Timestamp,
			CpuPercent:       record.CpuPercent,
			MemUsedPercent:   record.MemUsedPercent,
			DiskUtilPercent:  record.DiskUtilPercent,
			LoadAvg_1:        record.LoadAvg_1,
			Score:            record.Score,
		})
	}

	return response, nil
}

func (q *QueryManager) QueryLatestScore() (*proto.QueryLatestScoreResponse, error) {
	var servers []ServerScoreSummary
	var totalServers, onlineServers int64
	var avgScore float64

	// 获取每个服务器的最新记录
	rows, err := q.db.Raw(`
		SELECT server_name, MAX(timestamp) as last_update
		FROM performance_records
		GROUP BY server_name
	`).Rows()
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	for rows.Next() {
		var serverName string
		var lastUpdate time.Time
		if err := rows.Scan(&serverName, &lastUpdate); err != nil {
			continue
		}

		// 获取该服务器的最新性能数据
		var record PerformanceRecord
		q.db.Where("server_name = ? AND timestamp = ?", serverName, lastUpdate).First(&record)

		servers = append(servers, ServerScoreSummary{
			ServerName:      serverName,
			Score:           record.Score,
			LastUpdate:      lastUpdate,
			Status:          "ONLINE", // 简化处理，实际应根据心跳判断
			CpuPercent:      record.CpuPercent,
			MemUsedPercent:  record.MemUsedPercent,
			DiskUtilPercent: record.DiskUtilPercent,
			LoadAvg_1:       record.LoadAvg_1,
		})
	}

	// 计算集群统计信息
	q.db.Model(&PerformanceRecord{}).Distinct("server_name").Count(&totalServers)
	onlineServers = totalServers // 简化处理

	if len(servers) > 0 {
		q.db.Model(&PerformanceRecord{}).Select("AVG(score)").Scan(&avgScore)
	}

	// 转换为proto响应
	response := &proto.QueryLatestScoreResponse{
		ClusterStats: &proto.ClusterStats{
			TotalServers:   int32(totalServers),
			OnlineServers:  int32(onlineServers),
			OfflineServers: int32(totalServers - onlineServers),
			AvgScore:       float32(avgScore),
		},
	}

	for _, server := range servers {
		response.Servers = append(response.Servers, &proto.ServerScoreSummary{
			ServerName:      server.ServerName,
			Score:           server.Score,
			LastUpdate:      server.LastUpdate,
			Status:          proto.ServerStatus_ONLINE,
			CpuPercent:      server.CpuPercent,
			MemUsedPercent:  server.MemUsedPercent,
			DiskUtilPercent: server.DiskUtilPercent,
			LoadAvg_1:       server.LoadAvg_1,
		})
	}

	return response, nil
}

func (q *QueryManager) QueryAnomaly(req *proto.QueryAnomalyRequest) (*proto.QueryAnomalyResponse, error) {
	var anomalies []AnomalyRecord
	var totalCount int64

	// 构建查询
	query := q.db.Model(&AnomalyRecord{})
	if req.ServerName != "" {
		query = query.Where("server_name = ?", req.ServerName)
	}
	if req.TimeRange != nil {
		query = query.Where("timestamp >= ? AND timestamp <= ?", req.TimeRange.StartTime.AsTime(), req.TimeRange.EndTime.AsTime())
	}

	// 计算总数
	query.Count(&totalCount)

	// 分页
	page := int(req.Pagination.Page)
	pageSize := int(req.Pagination.PageSize)
	offset := (page - 1) * pageSize

	// 执行查询
	query.Order("timestamp DESC").Offset(offset).Limit(pageSize).Find(&anomalies)

	// 转换为proto响应
	response := &proto.QueryAnomalyResponse{
		TotalCount: int32(totalCount),
		Page:       req.Pagination.Page,
		PageSize:   req.Pagination.PageSize,
	}

	for _, anomaly := range anomalies {
		response.Anomalies = append(response.Anomalies, &proto.AnomalyRecord{
			ServerName:   anomaly.ServerName,
			Timestamp:    anomaly.Timestamp,
			AnomalyType:  anomaly.AnomalyType,
			Severity:     anomaly.Severity,
			Value:        anomaly.Value,
			Threshold:    anomaly.Threshold,
			MetricName:   anomaly.MetricName,
		})
	}

	return response, nil
}package service

import (
	"time"

	"github.com/monitor_system_go/proto"
	"gorm.io/gorm"
)

type QueryManager struct {
	db *gorm.DB
}

func NewQueryManager(db *gorm.DB) *QueryManager {
	return &QueryManager{
		db: db,
	}
}

func (q *QueryManager) QueryPerformance(req *proto.QueryPerformanceRequest) (*proto.QueryPerformanceResponse, error) {
	var records []PerformanceRecord
	var totalCount int64

	// 构建查询
	query := q.db.Model(&PerformanceRecord{})
	if req.ServerName != "" {
		query = query.Where("server_name = ?", req.ServerName)
	}
	if req.TimeRange != nil {
		query = query.Where("timestamp >= ? AND timestamp <= ?", req.TimeRange.StartTime.AsTime(), req.TimeRange.EndTime.AsTime())
	}

	// 计算总数
	query.Count(&totalCount)

	// 分页
	page := int(req.Pagination.Page)
	pageSize := int(req.Pagination.PageSize)
	offset := (page - 1) * pageSize

	// 执行查询
	query.Order("timestamp DESC").Offset(offset).Limit(pageSize).Find(&records)

	// 转换为proto响应
	response := &proto.QueryPerformanceResponse{
		TotalCount: int32(totalCount),
		Page:       req.Pagination.Page,
		PageSize:   req.Pagination.PageSize,
	}

	for _, record := range records {
		response.Records = append(response.Records, &proto.PerformanceRecord{
			ServerName:      record.ServerName,
			Timestamp:       record.Timestamp,
			CpuPercent:      record.CpuPercent,
			MemUsedPercent:  record.MemUsedPercent,
			DiskUtilPercent: record.DiskUtilPercent,
			LoadAvg_1:       record.LoadAvg_1,
			Score:           record.Score,
		})
	}

	return response, nil
}

func (q *QueryManager) QueryLatestScore() (*proto.QueryLatestScoreResponse, error) {
	var servers []ServerScoreSummary
	var totalServers, onlineServers int64
	var avgScore float64

	// 获取每个服务器的最新记录
	rows, err := q.db.Raw(`
		SELECT server_name, MAX(timestamp) as last_update
		FROM performance_records
		GROUP BY server_name
	`).Rows()
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	for rows.Next() {
		var serverName string
		var lastUpdate time.Time
		if err := rows.Scan(&serverName, &lastUpdate); err != nil {
			continue
		}

		// 获取该服务器的最新性能数据
		var record PerformanceRecord
		q.db.Where("server_name = ? AND timestamp = ?", serverName, lastUpdate).First(&record)

		servers = append(servers, ServerScoreSummary{
			ServerName:      serverName,
			Score:           record.Score,
			LastUpdate:      lastUpdate,
			Status:          "ONLINE", // 简化处理，实际应根据心跳判断
			CpuPercent:      record.CpuPercent,
			MemUsedPercent:  record.MemUsedPercent,
			DiskUtilPercent: record.DiskUtilPercent,
			LoadAvg_1:       record.LoadAvg_1,
		})
	}

	// 计算集群统计信息
	q.db.Model(&PerformanceRecord{}).Distinct("server_name").Count(&totalServers)
	onlineServers = totalServers // 简化处理

	if len(servers) > 0 {
		q.db.Model(&PerformanceRecord{}).Select("AVG(score)").Scan(&avgScore)
	}

	// 转换为proto响应
	response := &proto.QueryLatestScoreResponse{
		ClusterStats: &proto.ClusterStats{
			TotalServers:   int32(totalServers),
			OnlineServers:  int32(onlineServers),
			OfflineServers: int32(totalServers - onlineServers),
			AvgScore:       float32(avgScore),
		},
	}

	for _, server := range servers {
		response.Servers = append(response.Servers, &proto.ServerScoreSummary{
			ServerName:      server.ServerName,
			Score:           server.Score,
			LastUpdate:      server.LastUpdate,
			Status:          proto.ServerStatus_ONLINE,
			CpuPercent:      server.CpuPercent,
			MemUsedPercent:  server.MemUsedPercent,
			DiskUtilPercent: server.DiskUtilPercent,
			LoadAvg_1:       server.LoadAvg_1,
		})
	}

	return response, nil
}

func (q *QueryManager) QueryAnomaly(req *proto.QueryAnomalyRequest) (*proto.QueryAnomalyResponse, error) {
	var anomalies []AnomalyRecord
	var totalCount int64

	// 构建查询
	query := q.db.Model(&AnomalyRecord{})
	if req.ServerName != "" {
		query = query.Where("server_name = ?", req.ServerName)
	}
	if req.TimeRange != nil {
		query = query.Where("timestamp >= ? AND timestamp <= ?", req.TimeRange.StartTime.AsTime(), req.TimeRange.EndTime.AsTime())
	}

	// 计算总数
	query.Count(&totalCount)

	// 分页
	page := int(req.Pagination.Page)
	pageSize := int(req.Pagination.PageSize)
	offset := (page - 1) * pageSize

	// 执行查询
	query.Order("timestamp DESC").Offset(offset).Limit(pageSize).Find(&anomalies)

	// 转换为proto响应
	response := &proto.QueryAnomalyResponse{
		TotalCount: int32(totalCount),
		Page:       req.Pagination.Page,
		PageSize:   req.Pagination.PageSize,
	}

	for _, anomaly := range anomalies {
		response.Anomalies = append(response.Anomalies, &proto.AnomalyRecord{
			ServerName:  anomaly.ServerName,
			Timestamp:   anomaly.Timestamp,
			AnomalyType: anomaly.AnomalyType,
			Severity:    anomaly.Severity,
			Value:       anomaly.Value,
			Threshold:   anomaly.Threshold,
			MetricName:  anomaly.MetricName,
		})
	}

	return response, nil
}