#pragma once
#include<grpc++\grpc++.h>
#include"message.grpc.pb.h"
#include"const.h"
#include"Singleton.h"
 
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::VarifyService;
using message::GetVarifyReq;
using message::GetVarifyRsp;

class RPConPool {
public:
	RPConPool(size_t poolsize,std::string host,std::string port) :
	poolsize_(poolsize),host_(host),port_(port),b_stop_(false)
	{
		for (size_t i = 0; i < poolsize_; i++) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+port,
				grpc::InsecureChannelCredentials());
			connections_.push( VarifyService::NewStub(channel));
		}
	}
	~RPConPool() {
		std::lock_guard<std::mutex>lock(mutex_);
		Close();
		while(!connections_.empty()){
			connections_.pop();
		}
	}
	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}
	std::unique_ptr<VarifyService::Stub>getConnection() {
		std::unique_lock<std::mutex>lock(mutex_);
		cond_.wait(lock, [this]() 
			{
			if(b_stop_){
				return true;
			}
			return !connections_.empty();
			});
		if(b_stop_){
			return nullptr;
		}
		auto context=std::move(connections_.front());
		connections_.pop();
		return context;
	}
	void returnConnection(std::unique_ptr<VarifyService::Stub>context) {
		std::lock_guard<std::mutex>lock(mutex_);
		if(b_stop_){
			return;
		}
		connections_.push(std::move(context));
		cond_.notify_one();

	}
private:
	std::atomic<bool>b_stop_;
	size_t poolsize_;
	std::string host_, port_;
	std::queue<std::unique_ptr<VarifyService::Stub>>connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};
class VerifyGrpcClient :public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVarifyRsp GetVarifyCode(const std::string& email) {
		ClientContext context;
		GetVarifyRsp reply;
		GetVarifyReq request;
		request.set_email(email);
		auto stub_ = pool_->getConnection();
		Status status = stub_->GetVarifyCode(&context, request, &reply);
		if(status.ok()){
			pool_->returnConnection(std::move(stub_));
			return reply;
		}
		else {
			reply.set_error(ErrorCodes::RPCFailed);
			pool_->returnConnection(std::move(stub_));
			return reply;
		}

	}
private:
	VerifyGrpcClient();
	/*std::unique_ptr<VarifyService::Stub> stub_;*/
	std::unique_ptr<RPConPool>pool_;
};
/*核心机制：编译期的“身份绑定”
在传统的继承中，基类并不知道谁继承了它。但在 CRTP 中，基类通过模板参数 T 提前获知了派生类的类型。
类型注入：当编译器解析 Singleton<VerifyGrpcClient> 时，基类内部的 _instance 实际上被定义成了 std::shared_ptr<VerifyGrpcClient>。
权限桥梁：由于 VerifyGrpcClient 将其构造函数设为 private，外部无法直接实例化。
但通过 friend class Singleton<VerifyGrpcClient>;，基类获得了调用子类私有构造函数的“特权”。*/
