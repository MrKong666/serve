#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include "const.h"
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include<atomic>
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;


/**
 * ChatConPool 类：gRPC 客户端连接池
 * 作用：预先创建指定数量的 gRPC Stub 存储在队列中，实现连接的复用与线程安全管理
 */
class ChatConPool {
public:
    // 构造函数：初始化连接池并预创建连接
    ChatConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
        for (size_t i = 0; i < poolSize_; ++i) {
            // 1. 创建 gRPC 通道 (Channel)
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
                grpc::InsecureChannelCredentials());

            // 2. 根据通道创建服务存根 (Stub)，并存入队列
            // 注意：Stub 通常封装在 unique_ptr 中，代表独占权
            connections_.push(ChatService::NewStub(channel));
        }
    }

    // 析构函数：释放资源
    ~ChatConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        Close(); // 设置停止标志并唤醒所有等待线程
        while (!connections_.empty()) {
            connections_.pop(); // 清空存根队列
        }
    }

    /**
     * 获取连接 (以阻塞方式)
     * @return 成功返回 Stub 指针，池空时等待；如果池已关闭则返回 nullptr
     */
    std::unique_ptr<ChatService::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);

        // 使用条件变量等待：连接池不为空 或 池已停止
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !connections_.empty();
            });

        // 如果连接池已停止，直接返回空指针
        if (b_stop_) {
            return nullptr;
        }

        // 从队列头部取出一个 Stub 指针
        auto context = std::move(connections_.front());
        connections_.pop();
        return context; // 返回给调用者使用（所有权转移）
    }

    /**
     * 归还连接
     * @param context 使用完毕的 Stub 存根
     */
    void returnConnection(std::unique_ptr<ChatService::Stub> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return; // 如果池已停止，则不再接受归还，Stub 会随 context 销毁而自动释放
        }

        connections_.push(std::move(context)); // 将存根放回队列末尾
        cond_.notify_one(); // 唤醒一个正在等待连接的线程
    }

    /**
     * 关闭连接池
     */
    void Close() {
        b_stop_ = true;       // 设置停止标志
        cond_.notify_all();   // 唤醒所有正在 getConnection 处阻塞的线程
    }

private: 
    std::atomic<bool> b_stop_;    // 连接池停止标志，原子类型保证可见性
    size_t poolSize_;        // 池的大小
    std::string host_;       // 服务器地址
    std::string port_;       // 服务器端口

    // 存储连接存根的同步队列
    std::queue<std::unique_ptr<ChatService::Stub>> connections_;

    std::mutex mutex_;            // 互斥锁，保护队列操作
    std::condition_variable cond_; // 条件变量，实现生产者-消费者模式
};
class ChatGrpcClient :public Singleton<ChatGrpcClient>
{
    friend class Singleton<ChatGrpcClient>;
public:
    ~ChatGrpcClient() {

    }

    AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
    AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
private:

    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};

