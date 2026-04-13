#include "MsgNode.h"

RecvNode::RecvNode(short max_len, short msg_id)
	: MsgNode(max_len), _msg_id(msg_id)
{
}

/**
 * @brief SendNode 构造实现：协议封包过程
 * 协议格式：| 消息ID (2字节) | 数据长度 (2字节) | 实际数据 (max_len字节) |
 */
SendNode::SendNode(const char* msg, short max_len, short msg_id)
	: MsgNode(max_len + HEAD_TOTAL_LEN), _msg_id(msg_id) // 4字节用于消息ID和数据长度
{
	//主机字节序转换为网络字节序，确保跨平台兼容性
	short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	memcpy(_data, &msg_id_host, HEAD_ID_LEN);
	//数据长度也转换为网络字节序
	short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	memcpy(_data + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);


	//将实际数据复制到缓冲区
	memcpy(_data + HEAD_DATA_LEN+HEAD_ID_LEN, msg, max_len);

}
