#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

class CacheServer;

/**
 * HTTP处理器类
 * 提供HTTP REST API接口，将HTTP请求转换为缓存操作
 * 支持的操作：
 * - GET /{key}: 获取缓存值
 * - POST /: 批量设置键值对（JSON格式）
 * - DELETE /{key}: 删除缓存项
 * - GET /health: 健康检查
 * 
 * 特性：
 * - 多线程处理客户端请求
 * - JSON格式的请求和响应
 * - URL解码支持
 * - 优雅的错误处理
 */
class HttpHandler {
public:
    /**
     * 构造函数
     * @param server 缓存服务器实例指针
     * @param port HTTP服务监听端口
     */
    HttpHandler(CacheServer* server, int port);
    
    /**
     * 析构函数，确保资源正确释放
     */
    ~HttpHandler();
    
    /**
     * 启动HTTP服务器
     */
    void start();
    
    /**
     * 停止HTTP服务器
     */
    void stop();
    
private:
    CacheServer* server_;           // 缓存服务器实例指针
    int port_;                      // HTTP服务监听端口
    std::atomic<bool> running_;     // 服务器运行状态标志
    std::thread server_thread_;     // 服务器主线程
    int server_fd_;                 // 服务器套接字文件描述符
    
    /**
     * 服务器主循环，监听和接受客户端连接
     */
    void serverLoop();
    
    /**
     * 处理单个客户端请求
     * @param client_fd 客户端套接字文件描述符
     */
    void handleRequest(int client_fd);
    
    /**
     * 解析HTTP请求
     * @param request 原始HTTP请求字符串
     * @param method 输出参数，HTTP方法
     * @param path 输出参数，请求路径
     * @param body 输出参数，请求体
     * @return 解析结果（保留用于扩展）
     */
    std::string parseHttpRequest(const std::string& request, std::string& method, std::string& path, std::string& body);
    
    /**
     * 创建HTTP响应
     * @param status_code HTTP状态码
     * @param content_type 内容类型
     * @param body 响应体
     * @return 完整的HTTP响应字符串
     */
    std::string createHttpResponse(int status_code, const std::string& content_type, const std::string& body);
    
    /**
     * URL解码
     * @param str 需要解码的URL编码字符串
     * @return 解码后的字符串
     */
    std::string urlDecode(const std::string& str);
};