package main

import (
	"flag"
	"log"
	"time"

	"github.com/monitor_system_go/worker/monitor"
	"github.com/monitor_system_go/worker/rpc"
)

func main() {
	// 解析命令行参数
	managerAddr := flag.String("manager", "localhost:50051", "Manager服务地址")
	interval := flag.Int("interval", 1, "采集间隔（秒）")
	flag.Parse()

	// 创建监控数据采集器
	collector := monitor.NewMetricCollector()
	defer collector.Stop()

	// 创建监控数据推送器
	pusher, err := rpc.NewMonitorPusher(*managerAddr)
	if err != nil {
		log.Fatalf("创建推送器失败: %v\n", err)
	}
	defer pusher.Close()

	log.Printf("Worker启动成功，监控间隔: %d秒，Manager地址: %s\n", *interval, *managerAddr)

	// 循环采集和推送数据
	ticker := time.NewTicker(time.Duration(*interval) * time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			// 采集监控数据
			info := collector.CollectAll()

			// 推送数据到Manager
			err := pusher.PushData(info)
			if err != nil {
				log.Printf("推送数据失败: %v\n", err)
			} else {
				log.Printf("推送数据成功: %s\n", info.Name)
			}
		}
	}
}
