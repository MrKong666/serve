#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <mutex>

// 引入 gRPC 相关命名空间和消息定义
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

/**
 * @brief 聊天服务器信息类
 * 存储单个 ChatServer 的连接信息及其负载状态
 */
class ChatServer {
public:
	// 默认构造函数，初始化为空
	ChatServer() :host(""), port(""), name(""), con_count(0) {}

	// 拷贝构造函数，用于对象间的属性复制
	ChatServer(const ChatServer& cs) :host(cs.host), port(cs.port), name(cs.name), con_count(cs.con_count) {}

	// 重载赋值运算符，处理对象赋值并包含自赋值检查
	ChatServer& operator=(const ChatServer& cs) {
		if (&cs == this) {
			return *this;
		}

		host = cs.host;
		name = cs.name;
		port = cs.port;
		con_count = cs.con_count;
		return *this;
	}

	std::string host;      // 服务器 IP 地址
	std::string port;      // 服务器 端口
	std::string name;      // 服务器 唯一标识名称
	int con_count;         // 当前连接负载数量
};

/**
 * @brief 状态服务实现类
 * 继承自 Protobuf 生成的 Service 基类，提供具体的 RPC 逻辑
 */
class StatusServiceImpl final : public StatusService::Service
{
public:
	// 构造函数：初始化服务器配置
	StatusServiceImpl();

	// RPC 接口：获取负载最优的聊天服务器地址
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;

	// RPC 接口：用户登录状态验证
	Status Login(ServerContext* context, const LoginReq* request,
		LoginRsp* reply) override;

private:
	/**
	 * @brief 将生成的 Token 写入 Redis 缓存
	 * @param uid 用户唯一 ID
	 * @param token 随机生成的验证令牌
	 */
	void insertToken(int uid, std::string token);

	/**
	 * @brief 负载均衡核心算法
	 * @return 返回当前最适合接入的 ChatServer 对象
	 */
	ChatServer getChatServer();

	// 存储所有可用聊天服务器的哈希表
	std::unordered_map<std::string, ChatServer> _servers;

	// 互斥锁，保证多线程环境下 _servers 操作的安全性
	std::mutex _server_mtx;

};