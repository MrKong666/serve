#include "RedisMgr.h"
#include"ConfigMgr.h"
RedisMgr::RedisMgr()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	_con_pool.reset(new RedisConPool(5, host.c_str(), std::stoi(port), pwd.c_str()));
}

RedisMgr::~RedisMgr()
{
	Close();
}

//bool RedisMgr::Connect(const std::string& host, int port)
//{
//	auto _connect = _con_pool->getConnection();
//	if (_connect == nullptr) {
//		return false;
//	}
//	if (_connect != nullptr && _connect->err) {
//		std::cout << "connect error: " << _connect->errstr << std::endl;
//		return false;
//	}
//	return true;
//}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
	auto _connect = _con_pool->getConnection();
		if (_connect == nullptr) {
		return false;
	}
	Defer	defer([ &_connect, this]() {

			_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "GET %s", key.c_str());
	if (_reply == nullptr) {
		std::cout << "Get" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	if(_reply->type != REDIS_REPLY_STRING) {
		freeReplyObject(_reply);
		std::cout << "Get" << key << "faileed\n";
		_con_pool->returnConnection(_connect);
		return false;
	}
	value = _reply->str;
	freeReplyObject(_reply);
	std::cout << "Get" << key << "succeed\n";
	_con_pool->returnConnection(_connect);
	return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value)
{
	auto _connect = _con_pool->getConnection();
	auto _reply = (redisReply*)redisCommand(_connect, "SET %s %s", key.c_str(), value.c_str());
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	if(_reply == nullptr) {
		std::cout << "Set" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	if (!(_reply->type == REDIS_REPLY_STATUS && (strcmp(_reply->str, "OK") == 0 || strcmp(_reply->str, "ok") == 0))) {
		std::cout << "Set" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	freeReplyObject(_reply);
	std::cout << "Set" << key <<" " <<value<< "succeed\n";
	_con_pool->returnConnection(_connect);
	return true;
}

bool RedisMgr::Auth(const std::string& password)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "AUTH %s", password.c_str());
	if(_reply->type == REDIS_REPLY_ERROR) {
		std::cout << "认证失败\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	else {
		std::cout << "认证成功\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return true;
	}
	 
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "LPUSH %s %s", key.c_str(), value.c_str());
	if (_reply == nullptr) {
		std::cout << "LPush" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	if(_reply->type != REDIS_REPLY_INTEGER||_reply->integer<=0) {
		std::cout << "LPush" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	std::cout << "LPush" << key <<" "<<value << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "LPOP %s", key.c_str());
	if (_reply == nullptr||_reply->type==REDIS_REPLY_NIL) {
		std::cout << "LPop" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	value = _reply->str;
	std::cout << "LPop" << key << " " << value << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "RPUSH %s %s", key.c_str(), value.c_str());
	if (_reply == nullptr) {
		std::cout << "RPush" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	if (_reply->type != REDIS_REPLY_INTEGER || _reply->integer <= 0) {
		std::cout << "RPush" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	std::cout << "RPush" << key << " " << value << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "RPOP %s", key.c_str());
	if (_reply == nullptr || _reply->type == REDIS_REPLY_NIL) {
		std::cout << "RPop" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	value = _reply->str;
	std::cout << "RPop" << key << " " << value << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);

	return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value)
{
	auto _connect = _con_pool->getConnection();

	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (_reply == nullptr) {
		std::cout << "HSet" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	std::cout << "HSet" << key << " " << hkey << " " << value << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = hkey;
	argvlen[2] = strlen(hkey);
	argv[3] = hvalue;
	argvlen[3] = hvaluelen;
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommandArgv(_connect,  4,argv,argvlen);
	if(_reply == nullptr||_reply->type!=REDIS_REPLY_INTEGER) {
		std::cout << "HSet" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	std::cout << "HSet" << key << " " << hkey << " " << std::string(hvalue, hvaluelen) << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "HGET %s %s", key.c_str(), hkey.c_str());
	if(_reply == nullptr||_reply->type==REDIS_REPLY_NIL) {
		std::cout << "HGet" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return "";
	}
	std::string value = _reply->str;
	std::cout << "HGet" << key << " " << hkey << " " << value << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return value;
}

bool RedisMgr::Del(const std::string& key)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "DEL %s", key.c_str());
	if(_reply == nullptr||_reply->type!=REDIS_REPLY_INTEGER) {
		std::cout << "Del" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	std::cout << "Del" << key << "succeed\n";
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
	auto _connect = _con_pool->getConnection();
	Defer	defer([&_connect, this]() {

		_con_pool->returnConnection(_connect);

		});
	auto _reply = (redisReply*)redisCommand(_connect, "EXISTS %s", key.c_str());
	if(_reply == nullptr||_reply->type!=REDIS_REPLY_INTEGER||_reply->integer==0) {
		std::cout << "ExistsKey" << key << "faileed\n";
		freeReplyObject(_reply);
		_con_pool->returnConnection(_connect);
		return false;
	}
	std::cout << "ExistsKey" << key << "succeed\n";	
	freeReplyObject(_reply);
	_con_pool->returnConnection(_connect);
	return true;
}
void RedisMgr::Close()
{
	_con_pool->Close();
}
