#pragma once

#include <grpcpp/grpcpp.h>
#include "cache.grpc.pb.h"
#include "consistent_hash.h"
#include <memory>
#include <unordered_map>
#include <mutex>

/**
 * gRPC客户端类
 * 负责与其他缓存节点进行gRPC通信，实现分布式缓存的节点间数据交换
 * 
 * 主要功能：
 * - 远程缓存操作（获取、设置、删除）
 * - 节点健康检查
 * - gRPC连接池管理
 * - 线程安全的并发访问
 * 
 * 设计特点：
 * - 连接复用：为每个节点维护长连接，避免频繁建立连接的开销
 * - 线程安全：使用互斥锁保护连接池的并发访问
 * - 错误处理：优雅处理网络异常和节点故障
 */
class GrpcClient {
public:
    /**
     * 构造函数
     * 初始化gRPC客户端和连接池
     */
    GrpcClient();
    
    // 缓存操作接口
    /**
     * 从远程节点获取缓存值
     * @param node 目标节点信息
     * @param key 缓存键
     * @param value 输出参数，存储获取到的值
     * @return 是否成功获取
     */
    bool get(const Node& node, const std::string& key, std::string& value);
    
    /**
     * 向远程节点设置缓存值
     * @param node 目标节点信息
     * @param key 缓存键
     * @param value 缓存值
     * @return 是否成功设置
     */
    bool set(const Node& node, const std::string& key, const std::string& value);
    
    /**
     * 从远程节点删除缓存项
     * @param node 目标节点信息
     * @param key 要删除的缓存键
     * @return 是否成功删除
     */
    bool del(const Node& node, const std::string& key);
    
    /**
     * 检查远程节点健康状态
     * @param node 目标节点信息
     * @return 节点是否健康
     */
    bool health(const Node& node);
    
private:
    // gRPC连接池：地址到服务存根的映射
    std::unordered_map<std::string, std::unique_ptr<cache::CacheService::Stub>> stubs_;
    // 保护连接池的互斥锁
    std::mutex stubs_mutex_;
    
    /**
     * 获取或创建到指定节点的gRPC服务存根
     * @param node 目标节点信息
     * @return gRPC服务存根指针
     */
    cache::CacheService::Stub* getStub(const Node& node);
    
    /**
     * 构建节点的gRPC连接地址
     * @param node 节点信息
     * @return 格式为"host:port"的地址字符串
     */
    std::string getNodeAddress(const Node& node) const;
};