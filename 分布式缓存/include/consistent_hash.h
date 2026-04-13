#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

/**
 * 节点结构体
 * 表示分布式系统中的一个服务节点，包含节点的基本信息和网络配置
 */
struct Node {
    std::string id;        // 节点的唯一标识符
    std::string host;      // 节点的主机地址（IP或域名）
    int grpc_port;         // gRPC服务端口号
    int http_port;         // HTTP服务端口号
    
    // 默认构造函数，初始化端口为0
    Node() : grpc_port(0), http_port(0) {}
    
    /**
     * 带参数的构造函数
     * @param node_id 节点ID
     * @param node_host 节点主机地址
     * @param grpc_p gRPC端口
     * @param http_p HTTP端口
     */
    Node(const std::string& node_id, const std::string& node_host, 
         int grpc_p, int http_p) 
        : id(node_id), host(node_host), grpc_port(grpc_p), http_port(http_p) {}
};

/**
 * 一致性哈希类
 * 实现了一致性哈希算法，用于分布式系统中的负载均衡和数据分片
 * 支持动态添加和删除节点，最小化数据迁移量
 * 
 * 核心特性：
 * 1. 使用虚拟节点提高数据分布均匀性
 * 2. 支持节点的动态增删
 * 3. 基于MD5哈希算法保证良好的分布性
 * 4. 环形结构实现顺时针查找
 */
class ConsistentHash {
public:
    /**
     * 构造函数
     * @param virtual_nodes 每个物理节点对应的虚拟节点数量，默认100个
     */
    ConsistentHash(int virtual_nodes = 100);
    
    /**
     * 向哈希环中添加节点
     * @param node 要添加的节点信息
     */
    void addNode(const Node& node);
    
    /**
     * 从哈希环中移除节点
     * @param node_id 要移除的节点ID
     */
    void removeNode(const std::string& node_id);
    
    /**
     * 根据键值获取对应的节点
     * @param key 要查找的键值
     * @return 负责处理该键值的节点
     */
    Node getNode(const std::string& key) const;
    
    /**
     * 获取所有节点信息
     * @return 包含所有节点的向量
     */
    std::vector<Node> getAllNodes() const;
    
    /**
     * 检查节点是否存在
     * @param node_id 节点ID
     * @return 节点是否存在
     */
    bool hasNode(const std::string& node_id) const;
    
private:
    int virtual_nodes_;                              // 每个物理节点的虚拟节点数量
    std::map<uint32_t, std::string> ring_;          // 哈希环，键为哈希值，值为节点ID
    std::unordered_map<std::string, Node> nodes_;   // 节点映射表，存储节点ID到节点信息的映射
    
    /**
     * 计算字符串的哈希值
     * @param str 输入字符串
     * @return 32位哈希值
     */
    uint32_t hash(const std::string& str) const;
    
    /**
     * 生成虚拟节点的键值
     * @param node_id 物理节点ID
     * @param index 虚拟节点索引
     * @return 虚拟节点的唯一标识符
     */
    std::string getVirtualNodeKey(const std::string& node_id, int index) const;
};