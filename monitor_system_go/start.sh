#!/bin/bash

# 启动manager
echo "启动manager..."
./bin/manager --grpc-port=50051 --http-port=8080 --db-host=localhost --db-port=5432 --db-user=postgres --db-password=postgres --db-name=monitor_system &

# 等待manager启动
sleep 3

# 启动worker
echo "启动worker..."
./bin/worker --manager=localhost:50051 --interval=1 &

echo "系统启动完成！"
echo "HTTP API地址: http://localhost:8080"
echo "gRPC服务地址: localhost:50051"