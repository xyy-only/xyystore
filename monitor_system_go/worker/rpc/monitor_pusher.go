package rpc

import (
	"context"
	"log"
	"time"

	"github.com/monitor_system_go/proto"
	"google.golang.org/grpc"
)

type MonitorPusher struct {
	client proto.GrpcManagerClient
	conn   *grpc.ClientConn
}

func NewMonitorPusher(managerAddr string) (*MonitorPusher, error) {
	conn, err := grpc.Dial(managerAddr, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		return nil, err
	}

	client := proto.NewGrpcManagerClient(conn)
	return &MonitorPusher{
		client: client,
		conn:   conn,
	}, nil
}

func (p *MonitorPusher) PushData(info *proto.MonitorInfo) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	_, err := p.client.SetMonitorInfo(ctx, info)
	if err != nil {
		log.Printf("推送监控数据失败: %v\n", err)
		return err
	}

	return nil
}

func (p *MonitorPusher) Close() error {
	return p.conn.Close()
}
