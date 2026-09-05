#include "consistent_hash.h"
#include <openssl/md5.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

/**
 * 一致性哈希构造函数
 * @param virtual_nodes 每个物理节点对应的虚拟节点数量，默认100个
 * 虚拟节点的作用是提高数据分布的均匀性，减少节点增删时的数据迁移量
 */
ConsistentHash::ConsistentHash(int virtual_nodes) : virtual_nodes_(virtual_nodes) {}

/**
 * 向一致性哈希环中添加节点
 * @param node 要添加的节点信息，包含节点ID、主机地址和端口
 * 该函数会为每个物理节点创建多个虚拟节点并分布在哈希环上
 * 虚拟节点的目的是让数据分布更加均匀，避免数据倾斜
 */
void ConsistentHash::addNode(const Node& node) {
    // 将节点信息存储到节点映射表中
    nodes_[node.id] = node;
    
    // 为该物理节点在哈希环上添加虚拟节点
    for (int i = 0; i < virtual_nodes_; ++i) {
        // 生成虚拟节点的唯一标识符
        std::string virtual_key = getVirtualNodeKey(node.id, i);
        // 计算虚拟节点在哈希环上的位置
        uint32_t hash_value = hash(virtual_key);
        // 将虚拟节点添加到哈希环中，映射到对应的物理节点ID
        ring_[hash_value] = node.id;
    }
}

/**
 * 从一致性哈希环中移除节点
 * @param node_id 要移除的节点ID
 * 该函数会移除指定节点的所有虚拟节点，并从节点映射表中删除节点信息
 * 移除节点后，原本映射到该节点的数据会重新分配到环上的下一个节点
 */
void ConsistentHash::removeNode(const std::string& node_id) {
    // 检查节点是否存在
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return; // 节点不存在，直接返回
    }
    
    // 从哈希环中移除该节点的所有虚拟节点
    for (int i = 0; i < virtual_nodes_; ++i) {
        // 重新生成虚拟节点的标识符
        std::string virtual_key = getVirtualNodeKey(node_id, i);
        // 计算虚拟节点的哈希值
        uint32_t hash_value = hash(virtual_key);
        // 从哈希环中删除该虚拟节点
        ring_.erase(hash_value);
    }
    
    // 从节点映射表中删除节点信息
    nodes_.erase(it);
}

/**
 * 根据给定的键值获取对应的节点
 * @param key 要查找的键值（通常是缓存的key或数据标识）
 * @return 负责处理该键值的节点信息
 * 该函数实现了一致性哈希的核心算法：顺时针查找最近的节点
 */
Node ConsistentHash::getNode(const std::string& key) const {
    // 检查哈希环是否为空
    if (ring_.empty()) {
        throw std::runtime_error("没有可用节点"); // 没有可用节点
    }
    
    // 计算键值的哈希值，确定其在哈希环上的位置
    uint32_t hash_value = hash(key);
    
    // 在哈希环上顺时针查找第一个大于等于该哈希值的虚拟节点
    auto it = ring_.lower_bound(hash_value);
    if (it == ring_.end()) {
        // 如果没找到，说明该键值应该映射到环上的第一个节点（环形结构）
        it = ring_.begin();
    }
    
    // 返回虚拟节点对应的物理节点信息
    return nodes_.at(it->second);
}

/**
 * 获取所有节点的信息
 * @return 包含所有节点信息的向量
 * 该函数用于获取当前哈希环中所有物理节点的完整信息
 */
std::vector<Node> ConsistentHash::getAllNodes() const {
    std::vector<Node> result;
    // 遍历节点映射表，收集所有节点信息
    for (const auto& pair : nodes_) {
        result.push_back(pair.second);
    }
    return result;
}

/**
 * 检查指定节点是否存在于哈希环中
 * @param node_id 要检查的节点ID
 * @return 如果节点存在返回true，否则返回false
 * 该函数用于验证节点的存在性，避免重复添加或删除不存在的节点
 */
bool ConsistentHash::hasNode(const std::string& node_id) const {
    return nodes_.find(node_id) != nodes_.end();
}

/**
 * 计算字符串的哈希值
 * @param str 要计算哈希值的字符串
 * @return 32位无符号整数哈希值
 * 使用MD5算法计算哈希值，取前4个字节作为32位整数
 * MD5保证了良好的分布性，适合一致性哈希的需求
 */
uint32_t ConsistentHash::hash(const std::string& str) const {
    unsigned char digest[MD5_DIGEST_LENGTH];
    // 使用MD5算法计算字符串的摘要
    MD5(reinterpret_cast<const unsigned char*>(str.c_str()), str.length(), digest);
    
    // 取MD5摘要的前4个字节转换为32位无符号整数
    uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
        result = (result << 8) | digest[i]; // 按字节组装成32位整数
    }
    
    return result;
}

/**
 * 生成虚拟节点的唯一标识符
 * @param node_id 物理节点的ID
 * @param index 虚拟节点的索引（0到virtual_nodes_-1）
 * @return 虚拟节点的唯一标识符字符串
 * 通过在节点ID后添加索引号来区分同一物理节点的不同虚拟节点
 * 格式："节点ID#索引号"，例如："node1#0", "node1#1"
 */
std::string ConsistentHash::getVirtualNodeKey(const std::string& node_id, int index) const {
    return node_id + "#" + std::to_string(index);
}