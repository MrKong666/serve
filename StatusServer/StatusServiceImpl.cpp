#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include <climits>

/**
 * @brief 生成唯一的 UUID 字符串
 * 用于作为用户登录聊天服务器的临时凭证 (Token)
 */
std::string generate_unique_string() {
	// 创建随机 UUID 生成器并生成 UUID
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// 将 UUID 对象转换为标准字符串
	std::string unique_string = to_string(uuid);

	return unique_string;
}

/**
 * @brief RPC 接口：获取可用的聊天服务器
 * 逻辑：选出负载最低的服务器，生成 Token 并关联 UID 存入 Redis
 */
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("status server has received :  ");

	// 1. 调用内部算法获取当前最优的聊天服务器实例
	const auto& server = getChatServer();
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);

	// 2. 生成唯一的登录令牌
	reply->set_token(generate_unique_string());

	// 3. 将 UID 和 Token 的对应关系持久化到 Redis，供后续登录验证
	insertToken(request->uid(), reply->token());

	return Status::OK;
}

/**
 * @brief 构造函数：初始化服务器列表
 * 从配置文件中解析所有可用的聊天服务器信息并存入内存映射表
 */
StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::Inst();
	// 获取配置文件中所有服务器的名字列表（以逗号分隔）
	auto server_list = cfg["chatservers"]["Name"];

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	// 解析逗号分隔的字符串
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	// 遍历每个服务器名，读取其详细的 Host、Port 等配置
	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server;
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		// 存入服务器映射表
		_servers[server.name] = server;
	}
}

/**
 * @brief 负载均衡算法实现
 * 策略：目前默认返回服务器列表中的第一个。预留了基于 Redis 实时连接数的负载计算逻辑
 */
ChatServer StatusServiceImpl::getChatServer() {
	std::lock_guard<std::mutex> guard(_server_mtx);
	auto minServer = _servers.begin()->second;
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		//不存在则默认设置为最大
		minServer.con_count = INT_MAX;
	}
	else {
		minServer.con_count = std::stoi(count_str);
	}


	// 使用范围基于for循环
	for (auto& server : _servers) {

		if (server.second.name == minServer.name) {
			continue;
		}

		auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}

		if (server.second.con_count < minServer.con_count) {
			minServer = server.second;
		}
	}

	return minServer;
}

/**
 * @brief RPC 接口：用户登录验证
 * 逻辑：当用户连接 ChatServer 时，由 ChatServer 调用此接口校验 Token 是否合法
 */
Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();

	std::string uid_str = std::to_string(uid);
	// 拼接 Redis 查询的 Key
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";

	// 1. 从 Redis 中获取该 UID 预留的 Token
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		// Redis 中没有该 UID 的记录，说明未请求过 GetChatServer 或已过期
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}

	// 2. 比对客户端上传的 Token 与 Redis 记录是否一致
	if (token_value != token) {
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}

	// 验证成功
	reply->set_error(ErrorCodes::Success);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}

/**
 * @brief 内部辅助函数：存储 Token
 * 将 Token 以 UID 为标识存储到 Redis 中
 */
void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	// 写入 Redis，用于后续 Login 接口校验
	RedisMgr::GetInstance()->Set(token_key, token);
}