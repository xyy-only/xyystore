# 分布式缓存系统构建配置
# 基于CMake的C++项目构建脚本，配置依赖库和编译选项
cmake_minimum_required(VERSION 3.16)
project(distributed_cache)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找项目所需的依赖包
find_package(Protobuf REQUIRED)  # Protocol Buffers序列化库
find_package(Threads REQUIRED)   # 多线程支持库
find_package(PkgConfig REQUIRED) # pkg-config工具
find_package(OpenSSL REQUIRED)   # OpenSSL加密库

# 查找JSON处理库jsoncpp
# 优先使用pkg-config查找，如果失败则直接查找库文件
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
    find_library(JSONCPP_LIBRARY jsoncpp)
    if(NOT JSONCPP_LIBRARY)
        message(FATAL_ERROR "jsoncpp library not found")
    endif()
endif()

# 直接查找gRPC相关库文件
# gRPC是高性能RPC框架，用于节点间通信
find_library(GRPC_LIBRARY grpc++)                              # gRPC C++核心库
find_library(GRPC_UNSECURE_LIBRARY grpc++_unsecure)           # gRPC非安全连接库
find_library(GPR_LIBRARY gpr)                                 # gRPC平台抽象层
find_library(GRPC_CPP_REFLECTION_LIBRARY grpc++_reflection)   # gRPC反射库

if(NOT GRPC_LIBRARY)
    message(FATAL_ERROR "gRPC library not found")
endif()

# 设置头文件包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)  # 项目头文件目录
include_directories(${CMAKE_CURRENT_BINARY_DIR})         # 生成文件目录（protobuf生成的文件）

# 生成protobuf和gRPC文件
# 定义需要编译的.proto文件列表
set(PROTO_FILES
    proto/cache.proto  # 缓存服务的protobuf定义文件
)

# 遍历每个proto文件，生成对应的C++代码
foreach(proto_file ${PROTO_FILES})
    # 提取文件名（不含扩展名）和目录路径
    get_filename_component(proto_name ${proto_file} NAME_WE)
    get_filename_component(proto_path ${proto_file} DIRECTORY)
    
    # 定义生成的文件路径
    set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")      # protobuf生成的源文件
    set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")       # protobuf生成的头文件
    set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc")  # gRPC生成的源文件
    set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h")   # gRPC生成的头文件
    
    # 添加自定义命令来生成protobuf和gRPC代码
    add_custom_command(
        OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
        COMMAND protoc  # 调用protobuf编译器
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"                    # gRPC输出目录
             --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"                     # C++输出目录
             -I "${CMAKE_CURRENT_SOURCE_DIR}/${proto_path}"              # proto文件搜索路径
             --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`            # gRPC插件路径
             "${CMAKE_CURRENT_SOURCE_DIR}/${proto_file}"                 # 输入的proto文件
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${proto_file}"             # 依赖的proto文件
    )
    
    # 将生成的文件添加到对应的列表中
    list(APPEND PROTO_SRCS ${proto_srcs})  # 添加protobuf源文件
    list(APPEND PROTO_HDRS ${proto_hdrs})  # 添加protobuf头文件
    list(APPEND GRPC_SRCS ${grpc_srcs})    # 添加gRPC源文件
    list(APPEND GRPC_HDRS ${grpc_hdrs})    # 添加gRPC头文件
endforeach()

# 定义项目源文件列表
# 包含所有手写的C++源文件和生成的protobuf/gRPC文件
set(SOURCES
    src/main.cpp              # 主程序入口
    src/cache_server.cpp      # 缓存服务器实现
    src/consistent_hash.cpp   # 一致性哈希算法实现
    src/http_handler.cpp      # HTTP请求处理器
    src/grpc_client.cpp       # gRPC客户端实现
    ${PROTO_SRCS}             # 生成的protobuf源文件
    ${GRPC_SRCS}              # 生成的gRPC源文件
)

# 创建可执行文件
# 将所有源文件编译成名为cache_server的可执行程序
add_executable(cache_server ${SOURCES})

# 链接所需的库文件
# 将项目依赖的各种库链接到可执行文件中
target_link_libraries(cache_server
    ${GRPC_LIBRARY}                    # gRPC核心库
    ${GRPC_UNSECURE_LIBRARY}           # gRPC非安全连接库
    ${GPR_LIBRARY}                     # gRPC平台抽象层
    ${GRPC_CPP_REFLECTION_LIBRARY}     # gRPC反射库
    ${Protobuf_LIBRARIES}              # Protocol Buffers库
    OpenSSL::Crypto                    # OpenSSL加密库
    Threads::Threads                   # 线程库
)

# 链接JSON处理库
# 根据jsoncpp的查找方式选择不同的链接方法
if(JSONCPP_FOUND)
    # 使用pkg-config找到的jsoncpp
    target_link_libraries(cache_server ${JSONCPP_LIBRARIES})
    target_include_directories(cache_server PRIVATE ${JSONCPP_INCLUDE_DIRS})
else()
    # 使用直接查找到的jsoncpp库文件
    target_link_libraries(cache_server ${JSONCPP_LIBRARY})
endif()

# 编译器优化标志，追求最大性能
# 配置Release模式的编译选项，启用各种性能优化
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto -ffast-math -funroll-loops")
target_compile_options(cache_server PRIVATE 
    -Wall -Wextra          # 启用额外的警告信息
    -O3                    # 最高级别的优化
    -march=native          # 针对当前CPU架构优化
    -mtune=native          # 针对当前CPU调优
    -flto                  # 链接时优化
    -ffast-math            # 快速数学运算（可能降低精度）
    -funroll-loops         # 循环展开优化
    -finline-functions     # 函数内联优化
    -fomit-frame-pointer   # 省略帧指针以提高性能
    -DNDEBUG              # 禁用调试断言
)

# 启用链接时优化（LTO）
# 在链接阶段进行跨模块优化，进一步提升性能
set_property(TARGET cache_server PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
