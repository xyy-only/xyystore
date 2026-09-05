#!/bin/bash

# 编译proto文件
echo "编译proto文件..."
cd proto
go get github.com/golang/protobuf/protoc-gen-go
go get google.golang.org/grpc/cmd/protoc-gen-go-grpc
protoc --go_out=. --go-grpc_out=. *.proto
cd ..

# 编译worker
echo "编译worker..."
go build -o bin/worker worker/main.go

# 编译manager
echo "编译manager..."
go build -o bin/manager manager/main.go

echo "编译完成！"