#include "head.h"


int main ( )
{
	std::cout << "�ͻ�������...\r";
	try
	{
		// ����������
		asio::io_context ioc;
		// ����˿�
		ip::tcp::endpoint remote_ep (ip::address::from_string ("127.0.0.1"), 10086);
		ip::tcp::socket sock (ioc);
		boost::system::error_code error = asio::error::host_not_found;
		// ����
		sock.connect (remote_ep, error);
		if (error)
		{
			std::cout << "connect failed, code is " << error.value ( )
				<< " error msg is " << error.message ( );
			return 0;
		}

		std::thread send_thread ([&sock] {
			while (true)
			{
				std::this_thread::sleep_for (2ms);
				const char* request = "hello world"; // ���͵���Ϣ
				int request_length = std::strlen (request); // ��Ϣ�ĳ���
				char send_data[MAX_LENGTH] = { 0 };
				// ��ʱ��ת�����ֽ���

				// ...
				std::memcpy (send_data, &request_length, 2);
				std::memcpy (send_data + 2, request, request_length);
				asio::write (sock, asio::buffer (send_data, request_length + 2));
			}
			});

		std::thread recv_thread ([&sock] {
			while (true)
			{
				std::this_thread::sleep_for (2ms);
				std::cout << "begin to receive..." << std::endl;
				char reply_head[HEAD_LENGTH];
				std::size_t reply_length =
					asio::read (sock, asio::buffer (reply_head, HEAD_LENGTH));
				short msglen = 0;
				std::memcpy (&msglen, reply_head, HEAD_LENGTH);
				// ��ʱ��ת�����ֽ���

				// ...
				char msg[MAX_LENGTH] = { 0 };
				std::size_t msg_length =
					asio::read (sock, asio::buffer (msg, msglen));

				std::cout << "Reply is: ";
				std::cout.write (msg, msglen) << std::endl;
				std::cout << "Reply len is " << msglen << std::endl;
			}
			});

		send_thread.join ( );
		recv_thread.join ( );

	}
	catch (const std::exception& err)
	{
		std::cerr << "Exception : " << err.what ( ) << std::endl;
	}

	std::system ("pause");
}