#include <iostream>
#include <thread>
#include <string>
#include <boost/asio.hpp>
#include "const.h"
#include "CServer.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

void run ( )
{
	try
	{
		asio::io_context ioc;
		CServer s (ioc, 10086);
		ioc.run ( );

	}
	catch (const std::exception& err)
	{
		std::clog << "exception is " << err.what ( ) << std::endl;
	}
	
	//asio::io_context ioc;


}


int main ( )
{
	std::cout << std::boolalpha;
	run ( );


	std::system ("pause");
}