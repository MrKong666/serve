#pragma once
#include"const.h"
#include<string>
#include<iostream>
#include<boost/asio.hpp>

using boost::asio::ip::tcp;
/**
 * @brief 消息节点基类
 * 负责原始内存缓冲区的管理（申请、释放、清理）
 */
class MsgNode
{
public:
	MsgNode(short max_len):_total_len(max_len),_cur_len(0)
	{
		_data = new char[max_len+1]();
		_data[max_len] = '\0'; // 确保数据缓冲区以 null 结尾，方便字符串处理
	}

	~MsgNode()
	{
		std::cout << "destruct MsgNode\n";
		delete[] _data;
	}
	void Clear() {
		memset(_data, 0, _total_len);
		_cur_len = 0;
	}
	short _cur_len;   // 当前已处理/写入的数据长度
	short _total_len; // 该节点允许的最大数据长度
	char* _data;      // 指向数据缓冲区的指针
};
/**
 * @brief 接收消息节点
 * 用于存储从网络读取到的数据
 */
class RecvNode : public MsgNode {
	friend class LogicSystem;
public:
	// 构造接收节点：指定数据体最大长度和消息ID
	RecvNode(short max_len, short msg_id);
private:
	short _msg_id; // 消息ID
};

/**
 * @brief 发送消息节点
 * 将业务数据封装成符合协议格式（Header + Data）的报文
 */
class SendNode : public MsgNode {
	friend class LogicSystem;
public:
	// 构造发送节点：自动计算总长度并填充协议头
	SendNode(const char* msg, short max_len, short msg_id);
private:
	short _msg_id;
};
