#include "cache_server.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>
#include <cstdlib>

// 全局服务器实例指针，用于信号处理函数中的优雅关闭
std::unique_ptr<CacheServer> server;

/**
 * 信号处理函数
 * @param signal 接收到的信号编号
 * 当程序接收到SIGINT（Ctrl+C）或SIGTERM信号时，优雅地关闭服务器
 * 确保资源正确释放和连接正常断开
 */
void signalHandler(int signal) {
    std::cout << "\n接收到信号 " << signal << "，正在关闭服务器..." << std::endl;
    if (server) {
        server->stop(); // 停止服务器运行
    }
    exit(0); // 退出程序
}

/**
 * 设置集群配置函数
 * @param server 当前服务器实例指针
 * @param node_id 当前节点的ID
 * 该函数在后台线程中运行，负责将其他节点添加到当前节点的一致性哈希环中
 * 实现分布式缓存集群的自动发现和配置
 */
void setupCluster(CacheServer* server, const std::string& node_id) {
    // 等待一段时间确保所有服务器都已启动
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 将其他节点添加到集群中（排除自己）
    if (node_id != "server1") {
        Node node1("server1", "server1", 50051, 9527);
        server->addNode(node1);
    }
    
    if (node_id != "server2") {
        Node node2("server2", "server2", 50052, 9528);
        server->addNode(node2);
    }
    
    if (node_id != "server3") {
        Node node3("server3", "server3", 50053, 9529);
        server->addNode(node3);
    }
    
    std::cout << "节点 " << node_id << " 的集群配置已完成" << std::endl;
}

/**
 * 主函数 - 分布式缓存服务器入口点
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出状态码
 * 该函数负责初始化和启动分布式缓存服务器，包括：
 * 1. 设置信号处理器以支持优雅关闭
 * 2. 从环境变量读取节点配置
 * 3. 启动gRPC和HTTP服务
 * 4. 配置集群节点
 */
int main(int argc, char* argv[]) {
    // 设置信号处理器，支持优雅关闭
    signal(SIGINT, signalHandler);   // 处理Ctrl+C信号
    signal(SIGTERM, signalHandler);  // 处理终止信号
    
    // 从环境变量获取节点配置，默认为server1
    std::string node_id = std::getenv("NODE_ID") ? std::getenv("NODE_ID") : "server1";
    std::string host = "0.0.0.0";  // 监听所有网络接口
    
    int grpc_port, http_port;  // gRPC和HTTP服务端口
    
    // 根据节点ID分配不同的端口号
    if (node_id == "server1") {
        grpc_port = 50051;  // server1的gRPC端口
        http_port = 9527;   // server1的HTTP端口
    } else if (node_id == "server2") {
        grpc_port = 50052;  // server2的gRPC端口
        http_port = 9528;   // server2的HTTP端口
    } else if (node_id == "server3") {
        grpc_port = 50053;  // server3的gRPC端口
        http_port = 9529;   // server3的HTTP端口
    } else {
        std::cerr << "未知的节点ID: " << node_id << std::endl;
        return 1;  // 退出程序
    }
    
    // 输出服务器启动信息
    std::cout << "正在启动缓存服务器..." << std::endl;
    std::cout << "节点ID: " << node_id << std::endl;
    std::cout << "gRPC端口: " << grpc_port << std::endl;
    std::cout << "HTTP端口: " << http_port << std::endl;
    
    try {
        // 创建并启动服务器实例
        server = std::make_unique<CacheServer>(node_id, host, grpc_port, http_port);
        server->start();  // 启动gRPC和HTTP服务
        
        // 在后台线程中设置集群配置
        std::thread cluster_thread(setupCluster, server.get(), node_id);
        cluster_thread.detach();  // 分离线程，让其在后台运行
        
        // 保持服务器运行状态
        std::cout << "服务器正在运行中。按Ctrl+C停止服务器。" << std::endl;
        
        // 主线程进入无限循环，保持程序运行
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;  // 返回错误状态码
    }
    
    return 0;  // 正常退出
}