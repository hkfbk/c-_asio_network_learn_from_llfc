#pragma once
#ifndef __CSESSION_H__
#define __CSSSSION_H__
#include <iostream>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include "MsgNode.h"
#include "CServer.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

class CServer;
class CSession :public std::enable_shared_from_this<CSession>
{
public:

	CSession (asio::io_context& _ioc, CServer* _server);
	~CSession ( ) { }

	auto GetSocket ( ) -> ip::tcp::socket&;
	auto GetUuid ( ) -> std::string;
	auto Start ( ) -> void;
	auto SharedSelf ( ) -> std::shared_ptr<CSession>;
	auto Close ( ) -> void;
	auto Send (char* _msg, int _max_length) -> void;
	


private:
	/*-----mem_func-----*/
	auto HandleRead (const boost::system::error_code& err,
		std::size_t bytes_transferred,
		std::shared_ptr<CSession> shared_self)
		->void;
	auto HandleWrite (const boost::system::error_code& err,
		std::shared_ptr<CSession> shared_self)
		-> void;

	/*-----mem_variable-----*/

	ip::tcp::socket m_sock;
	//	存储uuid	
	std::string m_uuid;
	//	数据缓冲区
	char m_data[MAX_LENGTH];
	// 服务器对象	
	CServer* m_server;
	// 是否关闭
	bool m_b_close;
	// 消息队列
	std::queue < std::shared_ptr<MsgNode>> m_send_que;
	// 锁
	std::mutex m_sed_lock;
	// 收到的消息节点指针
	std::shared_ptr<MsgNode> m_recv_msg_node;
	// 判断待处理信息是否为头部信息
	bool m_b_head_parse;
	// 收到的头部结构
	std::shared_ptr<MsgNode> m_recv_head_node;

};

#endif // !__CSESSION_H__


