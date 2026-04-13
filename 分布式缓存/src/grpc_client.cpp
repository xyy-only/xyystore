#include "grpc_client.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <iostream>

/**
 * gRPC客户端构造函数
 * 初始化gRPC客户端，用于与其他缓存节点通信
 */
GrpcClient::GrpcClient() {}

/**
 * 从远程节点获取缓存值
 * 通过gRPC调用远程节点的Get服务获取指定键的值
 * @param node 目标节点信息
 * @param key 要获取的缓存键
 * @param value 输出参数，存储获取到的值
 * @return 是否成功获取
 */
bool GrpcClient::get(const Node& node, const std::string& key, std::string& value) {
    // 获取或创建到目标节点的gRPC连接
    auto stub = getStub(node);
    if (!stub) {
        return false;
    }
    
    // 构建gRPC请求
    cache::GetRequest request;
    request.set_key(key);
    
    cache::GetResponse response;
    grpc::ClientContext context;
    
    // 发送gRPC请求
    grpc::Status status = stub->Get(&context, request, &response);
    
    // 检查响应状态和结果
    if (status.ok() && response.found()) {
        value = response.value();
        return true;
    }
    
    return false;
}

/**
 * 向远程节点设置缓存值
 * 通过gRPC调用远程节点的Set服务设置键值对
 * @param node 目标节点信息
 * @param key 要设置的缓存键
 * @param value 要设置的缓存值
 * @return 是否成功设置
 */
bool GrpcClient::set(const Node& node, const std::string& key, const std::string& value) {
    // 获取或创建到目标节点的gRPC连接
    auto stub = getStub(node);
    if (!stub) {
        return false;
    }
    
    // 构建gRPC请求
    cache::SetRequest request;
    request.set_key(key);
    request.set_value(value);
    
    cache::SetResponse response;
    grpc::ClientContext context;
    
    // 发送gRPC请求
    grpc::Status status = stub->Set(&context, request, &response);
    
    // 检查响应状态和结果
    return status.ok() && response.success();
}

/**
 * 从远程节点删除缓存项
 * 通过gRPC调用远程节点的Delete服务删除指定键
 * @param node 目标节点信息
 * @param key 要删除的缓存键
 * @return 是否成功删除
 */
bool GrpcClient::del(const Node& node, const std::string& key) {
    // 获取或创建到目标节点的gRPC连接
    auto stub = getStub(node);
    if (!stub) {
        return false;
    }
    
    // 构建gRPC请求
    cache::DeleteRequest request;
    request.set_key(key);
    
    cache::DeleteResponse response;
    grpc::ClientContext context;
    
    // 发送gRPC请求
    grpc::Status status = stub->Delete(&context, request, &response);
    
    // 检查响应状态和结果
    return status.ok() && response.success();
}

/**
 * 检查远程节点健康状态
 * 通过gRPC调用远程节点的Health服务检查节点是否正常运行
 * @param node 目标节点信息
 * @return 节点是否健康
 */
bool GrpcClient::health(const Node& node) {
    // 获取或创建到目标节点的gRPC连接
    auto stub = getStub(node);
    if (!stub) {
        return false;
    }
    
    // 构建gRPC请求
    cache::HealthRequest request;
    cache::HealthResponse response;
    grpc::ClientContext context;
    
    // 发送gRPC请求
    grpc::Status status = stub->Health(&context, request, &response);
    
    // 检查响应状态和健康状态
    return status.ok() && response.healthy();
}

/**
 * 获取或创建到指定节点的gRPC连接存根
 * 使用连接池管理gRPC连接，避免重复创建连接
 * @param node 目标节点信息
 * @return gRPC服务存根指针，失败时返回nullptr
 */
cache::CacheService::Stub* GrpcClient::getStub(const Node& node) {
    // 使用互斥锁保护连接池的线程安全
    std::lock_guard<std::mutex> lock(stubs_mutex_);
    
    std::string address = getNodeAddress(node);
    
    // 检查连接池中是否已存在到该地址的连接
    auto it = stubs_.find(address);
    if (it != stubs_.end()) {
        return it->second.get();
    }
    
    // 创建新的gRPC连接和存根
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto stub = cache::CacheService::NewStub(channel);
    
    // 保存存根指针并将其添加到连接池
    cache::CacheService::Stub* stub_ptr = stub.get();
    stubs_[address] = std::move(stub);
    
    return stub_ptr;
}

/**
 * 构建节点的gRPC地址
 * 将节点的主机和端口信息组合成gRPC连接地址
 * @param node 节点信息
 * @return 格式为"host:port"的地址字符串
 */
std::string GrpcClient::getNodeAddress(const Node& node) const {
    return node.host + ":" + std::to_string(node.grpc_port);
}