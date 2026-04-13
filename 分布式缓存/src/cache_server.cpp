#include "cache_server.h"
#include <grpcpp/server_builder.h>
#include <iostream>
#include <thread>

/**
 * 缓存服务器构造函数
 * @param node_id 节点唯一标识符
 * @param host 服务器主机地址
 * @param grpc_port gRPC服务端口
 * @param http_port HTTP服务端口
 * 初始化分布式缓存服务器的所有组件，包括一致性哈希环、gRPC客户端和HTTP处理器
 */
CacheServer::CacheServer(const std::string& node_id, const std::string& host, 
                         int grpc_port, int http_port)
    : node_id_(node_id), host_(host), grpc_port_(grpc_port), http_port_(http_port) {
    
    // 创建一致性哈希环，每个物理节点100个虚拟节点
    hash_ring_ = std::make_unique<ConsistentHash>(100);
    // 创建gRPC客户端，用于与其他节点通信
    grpc_client_ = std::make_unique<GrpcClient>();
    // 创建HTTP处理器，提供REST API接口
    http_handler_ = std::make_unique<HttpHandler>(this, http_port_);
    
    // 将自身节点添加到哈希环中
    Node self_node(node_id_, host_, grpc_port_, http_port_);
    hash_ring_->addNode(self_node);
}

/**
 * 缓存服务器析构函数
 * 确保服务器正确关闭，释放所有资源
 */
CacheServer::~CacheServer() {
    stop(); // 停止所有服务
}

/**
 * 启动缓存服务器
 * 启动gRPC服务器和HTTP服务器，开始接受客户端请求
 * gRPC用于节点间通信，HTTP用于客户端API访问
 */
void CacheServer::start() {
    // 启动gRPC服务器
    grpc::ServerBuilder builder;
    std::string server_address = host_ + ":" + std::to_string(grpc_port_);
    
    // 配置gRPC服务器监听地址和服务
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(this);  // 注册缓存服务
    
    // 构建并启动gRPC服务器
    grpc_server_ = builder.BuildAndStart();
    if (!grpc_server_) {
        std::cerr << "在 " << server_address << " 启动gRPC服务器失败" << std::endl;
        return;
    }
    
    std::cout << "gRPC服务器正在监听 " << server_address << std::endl;
    
    // 启动HTTP服务器
    http_handler_->start();
    
    std::cout << "缓存服务器 " << node_id_ << " 启动成功" << std::endl;
}

/**
 * 停止缓存服务器
 * 优雅地关闭HTTP和gRPC服务器，确保正在处理的请求完成
 */
void CacheServer::stop() {
    // 停止HTTP服务器
    if (http_handler_) {
        http_handler_->stop();
    }
    
    // 停止gRPC服务器
    if (grpc_server_) {
        grpc_server_->Shutdown();  // 发起关闭
        grpc_server_->Wait();      // 等待所有请求处理完成
    }
}

/**
 * 获取缓存值
 * @param key 缓存键
 * @param value 输出参数，存储获取到的值
 * @return 是否成功获取到值
 * 根据一致性哈希算法确定键值应该存储在哪个节点，如果是本地节点则直接访问，
 * 否则通过gRPC调用远程节点
 */
bool CacheServer::get(const std::string& key, std::string& value) {
    if (isLocalKey(key)) {
        // 键值属于本地节点，直接从本地缓存获取
        return getLocal(key, value);
    } else {
        // 键值属于远程节点，通过gRPC调用获取
        Node target_node = hash_ring_->getNode(key);
        return grpc_client_->get(target_node, key, value);
    }
}

/**
 * 设置缓存值
 * @param key 缓存键
 * @param value 要设置的值
 * @return 是否成功设置
 * 根据一致性哈希算法确定键值应该存储在哪个节点，如果是本地节点则直接设置，
 * 否则通过gRPC调用远程节点
 */
bool CacheServer::set(const std::string& key, const std::string& value) {
    if (isLocalKey(key)) {
        // 键值属于本地节点，直接设置到本地缓存
        return setLocal(key, value);
    } else {
        // 键值属于远程节点，通过gRPC调用设置
        Node target_node = hash_ring_->getNode(key);
        return grpc_client_->set(target_node, key, value);
    }
}

/**
 * 删除缓存值
 * @param key 要删除的缓存键
 * @return 是否成功删除
 * 根据一致性哈希算法确定键值存储在哪个节点，如果是本地节点则直接删除，
 * 否则通过gRPC调用远程节点
 */
bool CacheServer::del(const std::string& key) {
    if (isLocalKey(key)) {
        // 键值属于本地节点，直接从本地缓存删除
        return delLocal(key);
    } else {
        // 键值属于远程节点，通过gRPC调用删除
        Node target_node = hash_ring_->getNode(key);
        return grpc_client_->del(target_node, key);
    }
}

/**
 * 向集群添加节点
 * @param node 要添加的节点信息
 * 将新节点添加到一致性哈希环中，实现集群的动态扩展
 */
void CacheServer::addNode(const Node& node) {
    hash_ring_->addNode(node);
    std::cout << "已添加节点: " << node.id << " (" << node.host << ":" << node.grpc_port << ")" << std::endl;
}

/**
 * 从集群移除节点
 * @param node_id 要移除的节点ID
 * 从一致性哈希环中移除节点，实现集群的动态缩容
 */
void CacheServer::removeNode(const std::string& node_id) {
    hash_ring_->removeNode(node_id);
    std::cout << "已移除节点: " << node_id << std::endl;
}

/**
 * gRPC Get服务实现
 * @param context gRPC服务器上下文
 * @param request 获取请求，包含要查找的键
 * @param response 获取响应，包含查找结果和值
 * @return gRPC状态
 * 处理来自其他节点的获取请求，只在本地缓存中查找
 */
grpc::Status CacheServer::Get(grpc::ServerContext* context,
                              const cache::GetRequest* request,
                              cache::GetResponse* response) {
    std::string value;
    // 在本地缓存中查找键值
    bool found = getLocal(request->key(), value);
    
    // 设置响应结果
    response->set_found(found);
    if (found) {
        response->set_value(value);
    }
    
    return grpc::Status::OK;
}

/**
 * gRPC Set服务实现
 * @param context gRPC服务器上下文
 * @param request 设置请求，包含键值对
 * @param response 设置响应，包含操作结果
 * @return gRPC状态
 * 处理来自其他节点的设置请求，将键值对存储到本地缓存
 */
grpc::Status CacheServer::Set(grpc::ServerContext* context,
                              const cache::SetRequest* request,
                              cache::SetResponse* response) {
    // 在本地缓存中设置键值对
    bool success = setLocal(request->key(), request->value());
    response->set_success(success);
    
    return grpc::Status::OK;
}

/**
 * gRPC Delete服务实现
 * @param context gRPC服务器上下文
 * @param request 删除请求，包含要删除的键
 * @param response 删除响应，包含操作结果
 * @return gRPC状态
 * 处理来自其他节点的删除请求，从本地缓存中删除指定键值
 */
grpc::Status CacheServer::Delete(grpc::ServerContext* context,
                                 const cache::DeleteRequest* request,
                                 cache::DeleteResponse* response) {
    // 从本地缓存中删除键值
    bool success = delLocal(request->key());
    response->set_success(success);
    
    return grpc::Status::OK;
}

/**
 * gRPC Health服务实现
 * @param context gRPC服务器上下文
 * @param request 健康检查请求
 * @param response 健康检查响应，包含节点状态和ID
 * @return gRPC状态
 * 提供节点健康状态检查，用于集群监控和负载均衡
 */
grpc::Status CacheServer::Health(grpc::ServerContext* context,
                                 const cache::HealthRequest* request,
                                 cache::HealthResponse* response) {
    // 设置节点健康状态和ID
    response->set_healthy(true);
    response->set_node_id(node_id_);
    
    return grpc::Status::OK;
}

/**
 * 判断键值是否属于本地节点
 * @param key 要检查的键
 * @return 如果键值应该存储在本地节点返回true，否则返回false
 * 使用一致性哈希算法确定键值的归属节点
 */
bool CacheServer::isLocalKey(const std::string& key) const {
    try {
        // 通过一致性哈希算法获取键值对应的目标节点
        Node target_node = hash_ring_->getNode(key);
        return target_node.id == node_id_;  // 检查是否为当前节点
    } catch (const std::exception& e) {
        return true; // 如果没有可用节点，默认为本地处理
    }
}

/**
 * 从本地缓存获取值
 * @param key 缓存键
 * @param value 输出参数，存储获取到的值
 * @return 是否成功获取到值
 * 线程安全的本地缓存访问方法
 */
bool CacheServer::getLocal(const std::string& key, std::string& value) {
    // 使用互斥锁保证线程安全
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 在本地缓存中查找键值
    auto it = local_cache_.find(key);
    if (it != local_cache_.end()) {
        value = it->second;  // 找到则返回值
        return true;
    }
    
    return false;  // 未找到
}

/**
 * 向本地缓存设置值
 * @param key 缓存键
 * @param value 要设置的值
 * @return 是否成功设置（总是返回true）
 * 线程安全的本地缓存设置方法
 */
bool CacheServer::setLocal(const std::string& key, const std::string& value) {
    // 使用互斥锁保证线程安全
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 设置键值对到本地缓存
    local_cache_[key] = value;
    return true;  // 设置操作总是成功
}

/**
 * 从本地缓存删除值
 * @param key 要删除的缓存键
 * @return 是否成功删除（键存在时返回true，不存在时返回false）
 * 线程安全的本地缓存删除方法
 */
bool CacheServer::delLocal(const std::string& key) {
    // 使用互斥锁保证线程安全
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 在本地缓存中查找并删除键值
    auto it = local_cache_.find(key);
    if (it != local_cache_.end()) {
        local_cache_.erase(it);  // 找到则删除
        return true;
    }
    
    return false;  // 键不存在
}