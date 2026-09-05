# 分布式缓存系统Docker镜像构建文件
# 基于Ubuntu 20.04构建C++分布式缓存服务器
FROM ubuntu:20.04

# 设置环境变量，避免交互式安装提示
# 非交互式安装模式，避免安装过程中的用户交互
ENV DEBIAN_FRONTEND=noninteractive
# 设置时区为上海
ENV TZ=Asia/Shanghai

# 使用清华大学镜像源加速软件包下载
# 将默认的Ubuntu软件源替换为清华镜像源
RUN sed -i 's@//.*archive.ubuntu.com@//mirrors.tuna.tsinghua.edu.cn@g' /etc/apt/sources.list && \
    sed -i 's@//.*security.ubuntu.com@//mirrors.tuna.tsinghua.edu.cn@g' /etc/apt/sources.list

# 安装项目依赖包
# 包含编译工具、开发库和运行时依赖
# build-essential: C++编译工具链（gcc、g++、make等）
# cmake: CMake构建系统
# pkg-config: 包配置工具
# libssl-dev: OpenSSL开发库
# libgrpc++-dev: gRPC C++开发库
# libprotobuf-dev: Protocol Buffers开发库
# protobuf-compiler-grpc: gRPC protobuf编译器
# libjsoncpp-dev: JSON处理库
# curl: HTTP客户端工具（用于健康检查）
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libssl-dev \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    libjsoncpp-dev \
    curl \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /app

# 复制项目源代码到容器中
COPY . .

# 创建构建目录并编译项目
# 使用多核并行编译以提高构建速度
# make -j$(nproc): 使用所有可用CPU核心进行并行编译
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# 暴露服务端口
# HTTP端口：9527、9528、9529
# gRPC端口：50051、50052、50053
EXPOSE 9527 9528 9529 50051 50052 50053

# 设置容器启动时的默认命令
# 运行编译好的缓存服务器可执行文件
CMD ["./build/cache_server"]