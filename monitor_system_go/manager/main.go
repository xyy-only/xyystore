package main

import (
	"flag"
	"fmt"
	"log"
	"net"

	"github.com/gin-gonic/gin"
	"github.com/monitor_system_go/manager/model"
	"github.com/monitor_system_go/manager/rpc"
	"github.com/monitor_system_go/manager/service"
	"github.com/monitor_system_go/proto"
	"gorm.io/driver/postgres"
	"gorm.io/gorm"
	"google.golang.org/grpc"
)

func main() {
	// 解析命令行参数
	grpcPort := flag.Int("grpc-port", 50051, "gRPC服务端口")
	httpPort := flag.Int("http-port", 8080, "HTTP服务端口")
	dbHost := flag.String("db-host", "localhost", "数据库主机")
	dbPort := flag.Int("db-port", 5432, "数据库端口")
	dbUser := flag.String("db-user", "postgres", "数据库用户")
	dbPassword := flag.String("db-password", "postgres", "数据库密码")
	dbName := flag.String("db-name", "monitor_system", "数据库名称")
	flag.Parse()

	// 连接数据库
	dsn := fmt.Sprintf("host=%s port=%d user=%s password=%s dbname=%s sslmode=disable",
		*dbHost, *dbPort, *dbUser, *dbPassword, *dbName)
	db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{})
	if err != nil {
		log.Fatalf("连接数据库失败: %v\n", err)
	}

	// 自动迁移数据库表
	db.AutoMigrate(&model.PerformanceRecord{}, &model.AnomalyRecord{})

	// 创建服务实例
	hostManager := service.NewHostManager(db)
	queryManager := service.NewQueryManager(db)

	// 启动gRPC服务器
	grpcServer := grpc.NewServer()
	proto.RegisterGrpcManagerServer(grpcServer, rpc.NewGrpcServer(hostManager))

	// 监听gRPC端口
	grpcAddr := fmt.Sprintf(":%d", *grpcPort)
	grpcListener, err := net.Listen("tcp", grpcAddr)
	if err != nil {
		log.Fatalf("监听gRPC端口失败: %v\n", err)
	}

	// 启动HTTP服务器
	r := gin.Default()

	// 注册API路由
	r.GET("/api/performance", func(c *gin.Context) {
		serverName := c.Query("server_name")
		// 简化处理，实际应解析时间范围和分页参数
		req := &proto.QueryPerformanceRequest{
			ServerName: serverName,
			Pagination: &proto.Pagination{
				Page:      1,
				PageSize:  100,
			},
		}

		resp, err := queryManager.QueryPerformance(req)
		if err != nil {
			c.JSON(500, gin.H{"error": err.Error()})
			return
		}

		c.JSON(200, resp)
	})

	r.GET("/api/latest-score", func(c *gin.Context) {
		resp, err := queryManager.QueryLatestScore()
		if err != nil {
			c.JSON(500, gin.H{"error": err.Error()})
			return
		}

		c.JSON(200, resp)
	})

	r.GET("/api/anomaly", func(c *gin.Context) {
		serverName := c.Query("server_name")
		// 简化处理，实际应解析时间范围和分页参数
		req := &proto.QueryAnomalyRequest{
			ServerName: serverName,
			Pagination: &proto.Pagination{
				Page:      1,
				PageSize:  100,
			},
		}

		resp, err := queryManager.QueryAnomaly(req)
		if err != nil {
			c.JSON(500, gin.H{"error": err.Error()})
			return
		}

		c.JSON(200, resp)
	})

	// 启动HTTP服务器
	httpAddr := fmt.Sprintf(":%d", *httpPort)
	go func() {
		log.Printf("HTTP服务器启动在 %s\n", httpAddr)
		if err := r.Run(httpAddr); err != nil {
			log.Fatalf("启动HTTP服务器失败: %v\n", err)
		}
	}()

	// 启动gRPC服务器
	log.Printf("gRPC服务器启动在 %s\n", grpcAddr)
	if err := grpcServer.Serve(grpcListener); err != nil {
		log.Fatalf("启动gRPC服务器失败: %v\n", err)
	}
}