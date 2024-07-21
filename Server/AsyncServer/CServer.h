#pragma once
#ifndef __CSERVER_H__
#define __CSERVER_H__
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <queue>
#include <memory>
#include <mutex>
#include "const.h"
#include <map>
#include "CSession.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;


class CSession;
class CServer
{
public:
	CServer (asio::io_context& _ioc, UInt16 _port);



	auto ClearSession (std::string _uuid) -> void;

	
private:
	/*-----mem_func-----*/
	auto HeadleAcceptor (std::shared_ptr<CSession>, 
		const boost::system::error_code& err) 
		-> void;

	auto StartAccept ( ) -> void;

	/*-----mem_variable-----*/
	//  上下文
	asio::io_context& m_ioc;
	// 端口号
	unsigned short m_prot;
	// 连接器
	ip::tcp::acceptor m_acceptor;
	//	以uuid为关键字的图，存储会话
	std::map<std::string, std::shared_ptr<CSession>> m_session;


};

#endif // !__CSERVER_H__



