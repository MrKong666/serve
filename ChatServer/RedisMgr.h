#pragma once
#include"const.h"

class RedisConPool {
public:
	RedisConPool(size_t poolSize, const char* host, int port,const char* pwd) :
		poolSize_(poolSize), host_(host), port_(port), b_stop(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			redisContext* context = redisConnect(host_, port_);
			if (context == nullptr || context->err != 0) {
				if (context != nullptr) {
					redisFree(context);
				}
				continue;
			}
			auto reply = (redisReply*)redisCommand(context, "AUTH %s",pwd);
			if (reply->type == REDIS_REPLY_ERROR) {
				std::cout << "认证失败\n";
				freeReplyObject(reply);
				continue;
			}
			freeReplyObject(reply);
			std::cout << "连接成功\n";
			connections_.push(context);
		}
	}
	~RedisConPool() {
		std::lock_guard<std::mutex>lock(mutex_);
		while (!connections_.empty()) {
			auto* context = connections_.front();
			redisFree(context);
			connections_.pop();
		}
	}
	redisContext* getConnection() {
		std::unique_lock<std::mutex>lock(mutex_);
		cond_.wait(lock, [this]() {
			if (b_stop)return true;
			return !connections_.empty();
			});
		if (b_stop)return nullptr;
		auto* context = connections_.front();
		connections_.pop();
		return context;
	}
	void returnConnection(redisContext* context) {
		std::lock_guard<std::mutex>lock(mutex_);
		if (b_stop)return;
		connections_.push(context);
		cond_.notify_one();
	}
	void Close() {
		b_stop = true;
		cond_.notify_all();
	}
private:
	std::atomic<bool>b_stop;
	size_t poolSize_;
	const char* host_;
	int port_;
	std::queue<redisContext*> connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};
class RedisMgr:public Singleton<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	//bool Connect(const std::string& host, int port);

	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool Auth(const std::string& password);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue,size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool Del(const std::string& key);
	bool ExistsKey(const std::string& key);
	void Close();
private:
	RedisMgr();
	/*redisContext* _connect;
	redisReply* _reply;*/
	std::unique_ptr<RedisConPool>_con_pool;
};

