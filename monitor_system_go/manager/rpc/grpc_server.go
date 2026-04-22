package rpc

import (
	"context"
	"log"

	"github.com/monitor_system_go/manager/service"
	"github.com/monitor_system_go/proto"
	"google.golang.org/protobuf/types/known/emptypb"
)

type GrpcServer struct {
	proto.UnimplementedGrpcManagerServer
	hostManager *service.HostManager
}

func NewGrpcServer(hostManager *service.HostManager) *GrpcServer {
	return &GrpcServer{
		hostManager: hostManager,
	}
}

func (s *GrpcServer) SetMonitorInfo(ctx context.Context, info *proto.MonitorInfo) (*emptypb.Empty, error) {
	log.Printf("收到来自 %s 的监控数据\n", info.Name)

	// 处理监控数据
	err := s.hostManager.ProcessMonitorInfo(info)
	if err != nil {
		log.Printf("处理监控数据失败: %v\n", err)
		return nil, err
	}

	return &emptypb.Empty{}, nil
}

func (s *GrpcServer) GetMonitorInfo(ctx context.Context, empty *emptypb.Empty) (*proto.MonitorInfo, error) {
	// 暂未实现
	return nil, nil
}
