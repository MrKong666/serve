#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include "ConfigMgr.h"
#include "hiredis.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "AsioIOServicePool.h"
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "StatusServiceImpl.h"

/**
 * @brief 核心服务运行逻辑
 * 负责 gRPC 服务的构建、启动以及信号监听
 */
void RunServer() {
    // 1. 获取配置实例并构造服务器监听地址 (例如 "127.0.0.1:50051")
    auto& cfg = ConfigMgr::Inst();
    std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);

    // 2. 创建状态服务实现类的实例
    StatusServiceImpl service;

    // 3. 使用 gRPC 的 ServerBuilder 配置服务器
    grpc::ServerBuilder builder;
    // 监听指定的端口，并使用不加密的凭据 (InsecureServerCredentials)
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 注册刚才创建的服务实例
    builder.RegisterService(&service);

    // 4. 构建并正式启动 gRPC 服务器
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    /* --- 优雅关闭机制 --- */

    // 5. 创建 Boost.Asio 的 io_context 用于处理异步信号
    boost::asio::io_context io_context;

    // 6. 注册需要捕获的系统信号 (SIGINT: Ctrl+C, SIGTERM: 终止信号)
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

    // 7. 设置异步等待信号的回调函数
    signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
        if (!error) {
            std::cout << "Shutting down server..." << std::endl;
            // 接收到信号后，调用 Shutdown 停止接收新请求并处理完现有请求
            server->Shutdown();
        }
        });

    // 8. 在一个独立的脱离线程 (detached thread) 中运行信号监听循环
    std::thread([&io_context]() { io_context.run(); }).detach();

    // 9. 主线程在此阻塞，直到服务器关闭
    server->Wait();

    // 10. 服务器彻底退出后，停止信号上下文
    io_context.stop();
}

/**
 * @brief 程序主入口
 */
int main(int argc, char** argv) {
    try {
        // 启动服务器
        RunServer();
    }
    catch (std::exception const& e) {
        // 捕获并打印启动过程中可能产生的任何标准异常
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}