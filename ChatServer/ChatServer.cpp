// ChatServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include"LogicSystem.h"
#include<csignal>
#include<thread>
#include<mutex>
#include"AsioIOServicePool.h"
#include"CServer.h"
#include"ConfigMgr.h"
#include"RedisMgr.h"
#include "ChatServiceImpl.h"
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;
int main()
{
    // 获取配置管理实例，并读取当前服务器在集群中的唯一标识名称
    auto& cfg = ConfigMgr::Inst();
    auto server_name = cfg["SelfServer"]["Name"];

    try {
        // 1. 初始化 AsioIOServicePool 线程池，用于调度底层网络 I/O 任务
        auto pool = AsioIOServicePool::GetInstance();

        // 2. 负载均衡初始化：在分布式路由中心（Redis）中注册当前节点
        // 使用 Hash 结构存储，初始在线登录人数（LOGIN_COUNT）设为 "0"
        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");

        // 3. 构建并启动内部 RPC 服务（gRPC Server）
        // 该服务主要用于微服务节点间的通讯（如跨服务器消息转发、状态仲裁等）
        std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
        ChatServiceImpl service; // 业务层 RPC 接口实现类
        grpc::ServerBuilder builder;

        // 监听指定的 RPC 端口并注册服务实现
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        // 启动 gRPC 监听
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        std::cout << "RPC Server listening on " << server_address << std::endl;

        // 4. 异步运行：单独开启线程维护 gRPC 服务器的阻塞监听，防止阻塞主线程 I/O
        std::thread grpc_server_thread([&server]() {
            server->Wait();
            });

        // 5. 信号管控：设置异步信号处理器，捕获 SIGINT (Ctrl+C) 或 SIGTERM
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool, &server](auto, auto) {
            // 优雅停止流程：先停止 Asio 事件循环，再停止 I/O 线程池，最后关闭 RPC 服务
            io_context.stop();
            pool->stop();
            server->Shutdown();
            });

        // 6. 启动核心 TCP 长连接监听服务
        auto port_str = cfg["SelfServer"]["Port"];
        CServer s(io_context, atoi(port_str.c_str()));

        // 7. 进入主事件循环，开始处理海量客户端的并发连接与数据收发
        io_context.run();

        // 8. 资源清理：服务退出后，从 Redis 中移除当前节点的负载信息并注销连接
        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();

        // 等待 RPC 线程回收，完成整个服务的安全关停
        grpc_server_thread.join();
    }
    catch (std::exception& e) {
        // 异常处理机制：一旦发生崩溃，确保从 Redis 中清除负载信息，防止路由中心将新用户引流至失效节点
        std::cerr << "Exception: " << e.what() << endl;
        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
    }
}


