#include "http_handler.h"
#include "cache_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <regex>
#include <json/json.h>

/**
 * HTTP处理器构造函数
 * 初始化HTTP服务器，设置缓存服务器引用和监听端口
 * @param server 缓存服务器实例指针
 * @param port HTTP服务监听端口
 */
HttpHandler::HttpHandler(CacheServer* server, int port) 
    : server_(server), port_(port), running_(false), server_fd_(-1) {}

/**
 * HTTP处理器析构函数
 * 确保服务器正确停止并释放资源
 */
HttpHandler::~HttpHandler() {
    stop();
}

/**
 * 启动HTTP服务器
 * 设置运行标志并在新线程中启动服务器循环
 */
void HttpHandler::start() {
    running_ = true;
    server_thread_ = std::thread(&HttpHandler::serverLoop, this);
}

/**
 * 停止HTTP服务器
 * 设置停止标志，关闭套接字，等待服务器线程结束
 */
void HttpHandler::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

/**
 * HTTP服务器主循环
 * 创建套接字，绑定端口，监听连接，为每个客户端请求创建处理线程
 */
void HttpHandler::serverLoop() {
    // 创建TCP套接字
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "创建套接字失败" << std::endl;
        return;
    }
    
    // 设置套接字选项，允许地址重用
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 配置服务器地址结构
    struct sockaddr_in address;
    address.sin_family = AF_INET;           // IPv4协议族
    address.sin_addr.s_addr = INADDR_ANY;   // 监听所有可用接口
    address.sin_port = htons(port_);        // 设置监听端口（网络字节序）
    
    // 绑定套接字到指定地址和端口
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "绑定套接字到端口 " << port_ << " 失败" << std::endl;
        close(server_fd_);
        return;
    }
    
    // 开始监听连接，设置最大等待队列长度为10
    if (listen(server_fd_, 10) < 0) {
        std::cerr << "监听套接字失败" << std::endl;
        close(server_fd_);
        return;
    }
    
    std::cout << "HTTP服务器正在监听端口 " << port_ << std::endl;
    
    // 主服务循环：接受客户端连接并处理请求
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // 接受客户端连接
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "接受连接失败" << std::endl;
            }
            continue;
        }
        
        // 为每个客户端请求创建独立的处理线程
        std::thread(&HttpHandler::handleRequest, this, client_fd).detach();
    }
}

/**
 * 处理HTTP请求
 * 接收客户端请求，解析HTTP协议，根据请求类型调用相应的缓存操作
 * @param client_fd 客户端套接字文件描述符
 */
void HttpHandler::handleRequest(int client_fd) {
    // 读取客户端请求数据
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    // 确保字符串以null结尾
    buffer[bytes_read] = '\0';
    std::string request(buffer);
    
    // 解析HTTP请求，提取方法、路径和请求体
    std::string method, path, body;
    parseHttpRequest(request, method, path, body);
    
    std::string response;
    
    try {
        if (path == "/health") {
            // 健康检查端点：返回节点状态信息
            Json::Value json_response;
            json_response["healthy"] = true;
            json_response["node_id"] = server_->getNodeId();
            
            Json::StreamWriterBuilder builder;
            std::string json_str = Json::writeString(builder, json_response);
            response = createHttpResponse(200, "application/json", json_str);
        }
        else if (method == "POST" && path == "/") {
            // 设置操作：批量设置键值对
            Json::Value json_data;
            Json::Reader reader;
            
            if (reader.parse(body, json_data)) {
                Json::Value json_response;
                bool all_success = true;
                
                // 遍历JSON对象中的所有键值对
                for (const auto& key : json_data.getMemberNames()) {
                    Json::StreamWriterBuilder builder;
                    std::string value = Json::writeString(builder, json_data[key]);
                    // 移除简单字符串的引号
                    if (value.front() == '"' && value.back() == '"') {
                        value = value.substr(1, value.length() - 2);
                    }
                    
                    // 调用缓存服务器的设置方法
                    if (!server_->set(key, value)) {
                        all_success = false;
                    }
                }
                
                json_response["success"] = all_success;
                Json::StreamWriterBuilder builder;
                std::string json_str = Json::writeString(builder, json_response);
                response = createHttpResponse(200, "application/json", json_str);
            } else {
                // JSON解析失败
                Json::Value error_response;
                error_response["detail"] = "无效的JSON格式";
                Json::StreamWriterBuilder builder;
                std::string json_str = Json::writeString(builder, error_response);
                response = createHttpResponse(400, "application/json", json_str);
            }
        }
        else if (method == "GET" && path.length() > 1) {
            // 获取操作：根据键获取值
            std::string key = urlDecode(path.substr(1)); // 移除路径前的'/'
            std::string value;
            
            if (server_->get(key, value)) {
                // 成功获取到值，返回JSON格式响应
                Json::Value json_response;
                json_response[key] = value;
                Json::StreamWriterBuilder builder;
                std::string json_str = Json::writeString(builder, json_response);
                response = createHttpResponse(200, "application/json", json_str);
            } else {
                // 键不存在，返回404错误
                Json::Value error_response;
                error_response["detail"] = "未找到";
                Json::StreamWriterBuilder builder;
                std::string json_str = Json::writeString(builder, error_response);
                response = createHttpResponse(404, "application/json", json_str);
            }
        }
        else if (method == "DELETE" && path.length() > 1) {
            // 删除操作：根据键删除缓存项
            std::string key = urlDecode(path.substr(1)); // 移除路径前的'/'
            
            bool success = server_->del(key);
            // 返回简单的成功/失败标识
            response = createHttpResponse(200, "text/plain", success ? "1" : "0");
        }
        else {
            // 不支持的请求路径或方法
            Json::Value error_response;
            error_response["detail"] = "未找到";
            Json::StreamWriterBuilder builder;
            std::string json_str = Json::writeString(builder, error_response);
            response = createHttpResponse(404, "application/json", json_str);
        }
    } catch (const std::exception& e) {
        // 捕获所有异常，返回内部服务器错误
        Json::Value error_response;
        error_response["detail"] = "内部服务器错误";
        Json::StreamWriterBuilder builder;
        std::string json_str = Json::writeString(builder, error_response);
        response = createHttpResponse(500, "application/json", json_str);
    }
    
    // 发送响应并关闭连接
    send(client_fd, response.c_str(), response.length(), 0);
    close(client_fd);
}

/**
 * 解析HTTP请求
 * 从原始HTTP请求字符串中提取方法、路径和请求体
 * @param request 原始HTTP请求字符串
 * @param method 输出参数，HTTP方法（GET、POST、DELETE等）
 * @param path 输出参数，请求路径
 * @param body 输出参数，请求体内容
 * @return 空字符串（保留用于扩展）
 */
std::string HttpHandler::parseHttpRequest(const std::string& request, std::string& method, std::string& path, std::string& body) {
    std::istringstream iss(request);
    std::string line;
    
    // 解析请求行：提取HTTP方法和路径
    if (std::getline(iss, line)) {
        std::istringstream first_line(line);
        first_line >> method >> path;
    }
    
    // 跳过HTTP头部，寻找空行（头部和体的分隔符）
    bool in_body = false;
    while (std::getline(iss, line)) {
        if (line == "\r" || line.empty()) {
            in_body = true;
            break;
        }
    }
    
    // 读取请求体内容
    if (in_body) {
        std::string body_line;
        while (std::getline(iss, body_line)) {
            body += body_line;
        }
    }
    
    return "";
}

/**
 * 创建HTTP响应
 * 根据状态码、内容类型和响应体构建完整的HTTP响应
 * @param status_code HTTP状态码
 * @param content_type 内容类型（如application/json、text/plain）
 * @param body 响应体内容
 * @return 完整的HTTP响应字符串
 */
std::string HttpHandler::createHttpResponse(int status_code, const std::string& content_type, const std::string& body) {
    std::ostringstream oss;
    
    // 根据状态码确定状态文本
    std::string status_text;
    switch (status_code) {
        case 200: status_text = "成功"; break;
        case 400: status_text = "请求错误"; break;
        case 404: status_text = "未找到"; break;
        case 500: status_text = "内部服务器错误"; break;
        default: status_text = "未知"; break;
    }
    
    // 构建HTTP响应头
    oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    oss << "Content-Type: " << content_type << "\r\n";
    oss << "Content-Length: " << body.length() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";  // 头部和体之间的空行
    oss << body;     // 响应体
    
    return oss.str();
}

/**
 * URL解码
 * 将URL编码的字符串解码为原始字符串
 * 处理%编码的字符和+号转空格
 * @param str 需要解码的URL编码字符串
 * @return 解码后的原始字符串
 */
std::string HttpHandler::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // 处理%编码：将%后的两位十六进制数转换为字符
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;  // 跳过已处理的两位十六进制数
            } else {
                result += str[i];  // 解析失败，保持原字符
            }
        } else if (str[i] == '+') {
            // 将+号转换为空格
            result += ' ';
        } else {
            // 普通字符直接添加
            result += str[i];
        }
    }
    return result;
}