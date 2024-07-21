#include "CServer.h"

CServer::CServer (asio::io_context& _ioc, UInt16 _port)
	:m_ioc (_ioc), m_prot(_port),
	m_acceptor (_ioc, ip::tcp::endpoint (ip::tcp::v4 ( ), _port))
{
	std::cout << "Server start success, listen on port : " << _port << std::endl;
	StartAccept ( );
}

auto CServer::ClearSession (std::string _uuid) -> void
{
	m_session.erase (_uuid);
}


auto CServer::HeadleAcceptor (std::shared_ptr<CSession> _newSession,
	const boost::system::error_code& err)
	-> void
{
	if (!err)
	{
		_newSession->Start ( );
		m_session.insert (std::make_pair (_newSession->GetUuid ( ), _newSession));
	}
	else
		std::cerr << "session accept failed msg is  : " << err.message ( ) << std::endl;
	this->StartAccept ( );
}

auto CServer::StartAccept ( )
-> void
{
	std::shared_ptr<CSession> new_session{
		std::make_shared<CSession> (m_ioc, this)
	};
	this->m_acceptor.async_accept (new_session->GetSocket ( ),
		std::bind (&CServer::HeadleAcceptor, this,
			new_session, std::placeholders::_1));

}
