#pragma once

#include <grpcpp/grpcpp.h>
#include "cache.grpc.pb.h"
#include "consistent_hash.h"
#include "grpc_client.h"
#include "http_handler.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>

/**
 * 分布式缓存服务器类
 * 实现了一个基于一致性哈希的分布式缓存系统，支持：
 * 1. gRPC协议的节点间通信
 * 2. HTTP REST API的客户端访问
 * 3. 动态节点管理和负载均衡
 * 4. 线程安全的本地缓存操作
 * 5. 自动数据路由和分片
 */
class CacheServer : public cache::CacheService::Service {
public:
    /**
     * 构造函数
     * @param node_id 节点唯一标识符
     * @param host 服务器主机地址
     * @param grpc_port gRPC服务端口
     * @param http_port HTTP服务端口
     */
    CacheServer(const std::string& node_id, const std::string& host, 
                int grpc_port, int http_port);
    
    /**
     * 析构函数，确保资源正确释放
     */
    ~CacheServer();
    
    // 服务器生命周期管理
    /**
     * 启动服务器，开始监听gRPC和HTTP请求
     */
    void start();
    
    /**
     * 停止服务器，优雅关闭所有连接
     */
    void stop();
    
    // 缓存操作接口
    /**
     * 获取缓存值
     * @param key 缓存键
     * @param value 输出参数，存储获取到的值
     * @return 是否成功获取
     */
    bool get(const std::string& key, std::string& value);
    
    /**
     * 设置缓存值
     * @param key 缓存键
     * @param value 要设置的值
     * @return 是否成功设置
     */
    bool set(const std::string& key, const std::string& value);
    
    /**
     * 删除缓存值
     * @param key 要删除的缓存键
     * @return 是否成功删除
     */
    bool del(const std::string& key);
    
    // 节点管理
    /**
     * 向集群添加节点
     * @param node 要添加的节点信息
     */
    void addNode(const Node& node);
    
    /**
     * 从集群移除节点
     * @param node_id 要移除的节点ID
     */
    void removeNode(const std::string& node_id);
    
    // 访问器方法
    /**
     * 获取当前节点ID
     * @return 节点ID字符串
     */
    const std::string& getNodeId() const { return node_id_; }
    
    // gRPC服务接口实现
    /**
     * gRPC获取服务实现
     * @param context gRPC服务器上下文
     * @param request 获取请求
     * @param response 获取响应
     * @return gRPC状态
     */
    grpc::Status Get(grpc::ServerContext* context,
                     const cache::GetRequest* request,
                     cache::GetResponse* response) override;
    
    /**
     * gRPC设置服务实现
     * @param context gRPC服务器上下文
     * @param request 设置请求
     * @param response 设置响应
     * @return gRPC状态
     */
    grpc::Status Set(grpc::ServerContext* context,
                     const cache::SetRequest* request,
                     cache::SetResponse* response) override;
    
    /**
     * gRPC删除服务实现
     * @param context gRPC服务器上下文
     * @param request 删除请求
     * @param response 删除响应
     * @return gRPC状态
     */
    grpc::Status Delete(grpc::ServerContext* context,
                        const cache::DeleteRequest* request,
                        cache::DeleteResponse* response) override;
    
    /**
     * gRPC健康检查服务实现
     * @param context gRPC服务器上下文
     * @param request 健康检查请求
     * @param response 健康检查响应
     * @return gRPC状态
     */
    grpc::Status Health(grpc::ServerContext* context,
                        const cache::HealthRequest* request,
                        cache::HealthResponse* response) override;
    
private:
    // 节点基本信息
    std::string node_id_;    // 节点唯一标识符
    std::string host_;       // 服务器主机地址
    int grpc_port_;          // gRPC服务端口
    int http_port_;          // HTTP服务端口
    
    // 本地存储
    std::unordered_map<std::string, std::string> local_cache_;  // 本地缓存存储
    std::mutex cache_mutex_;                                     // 缓存访问互斥锁
    
    // 分布式组件
    std::unique_ptr<ConsistentHash> hash_ring_;   // 一致性哈希环
    std::unique_ptr<GrpcClient> grpc_client_;     // gRPC客户端，用于节点间通信
    std::unique_ptr<HttpHandler> http_handler_;   // HTTP处理器，提供REST API
    
    // gRPC服务器
    std::unique_ptr<grpc::Server> grpc_server_;   // gRPC服务器实例
    
    // 辅助方法
    /**
     * 判断键值是否属于本地节点
     * @param key 要检查的键
     * @return 是否为本地键值
     */
    bool isLocalKey(const std::string& key) const;
    
    /**
     * 从本地缓存获取值
     * @param key 缓存键
     * @param value 输出参数，存储获取到的值
     * @return 是否成功获取
     */
    bool getLocal(const std::string& key, std::string& value);
    
    /**
     * 向本地缓存设置值
     * @param key 缓存键
     * @param value 要设置的值
     * @return 是否成功设置
     */
    bool setLocal(const std::string& key, const std::string& value);
    
    /**
     * 从本地缓存删除值
     * @param key 要删除的缓存键
     * @return 是否成功删除
     */
    bool delLocal(const std::string& key);
};