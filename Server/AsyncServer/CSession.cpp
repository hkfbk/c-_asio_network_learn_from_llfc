#include "CSession.h"

CSession::CSession (asio::io_context& _ioc, CServer* _server)
	:m_sock(_ioc),m_server(_server),m_b_close(false),m_b_head_parse(false)
{ 
	//std::memcpy (m_data, 0, MAX_LENGTH);


	//	����uuid
	boost::uuids::uuid uuid =
		boost::uuids::random_generator ( )();
	m_uuid = boost::uuids::to_string (uuid);

	//	�洢��Ϣ�ڵ�
	m_recv_head_node =
		std::make_shared<MsgNode> (HEAD_LENGTH);

}

auto CSession::Start ( ) -> void
{
	std::memset(m_data, 0, MAX_LENGTH);
	this->m_sock.async_read_some (asio::buffer (m_data, MAX_LENGTH),
		std::bind (&CSession::HandleRead, this,
			std::placeholders::_1, std::placeholders::_2, this->SharedSelf()));
}

auto CSession::Close ( ) -> void
{
	this->m_sock.close ( );
	this->m_b_close = true;
}


auto CSession::SharedSelf ( ) -> std::shared_ptr<CSession>
{
	return shared_from_this();
}

auto CSession::GetSocket ( ) -> ip::tcp::socket&
{
	// TODO: �ڴ˴����� return ���
	return this->m_sock;
}

auto CSession::GetUuid ( ) -> std::string
{
	return m_uuid;
}

auto CSession::Send (char* _msg, int _max_length) -> void
{
	// ����
	std::lock_guard<std::mutex> lock (this->m_sed_lock);
	// ���г���
	auto send_que_size = this->m_send_que.size ( );
	// �ж��Ƿ񳬹�������󳤶�
	if (!send_que_size > MAX_SENDQUE)
	{
		std::cout << "session: " << this->m_uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	} // END if (!send_que_size > MAX_SENDQUE)

	// �����Ͷ�����뷢�Ͷ���
	this->m_send_que.push (std::make_shared<MsgNode> (_msg, _max_length));
	// �������Ƿ�Ϊ��
	if (send_que_size > 0)
		return;
	// ��ȡҪ���͵���Ϣ�ڵ�
	auto& msgnode = this->m_send_que.front ( );
	asio::async_write (this->m_sock, asio::buffer (msgnode->m_data, msgnode->m_total_len),
		std::bind (&CSession::HandleWrite, this,
			std::placeholders::_1, SharedSelf ( )));

}

auto CSession::HandleRead (const boost::system::error_code & err, 
	std::size_t bytes_transferred,
	std::shared_ptr<CSession> shared_self)
	-> void
{
	if (!err)//	����Ƿ����쳣������д�ӡ������־���ر����ӣ�
	{
		int copy_len = 0; // �����ѿ����ĳ���
		while (bytes_transferred > 0)
		{
			//	����ǲ���ͷ����Ϣ
			if (!this->m_b_head_parse)
			{
				// �ȴ����յ������ݲ���ͷ�����ݴ�С�����
				// ��ȡ����һ���֣�Ȼ�����ͷ��ʣ����٣������Ѵ����ȣ� ����read����
				if (bytes_transferred + this->m_recv_head_node->m_cur_len < HEAD_LENGTH)
				{
					// ����ͷ������
					std::memcpy (this->m_recv_head_node->m_data + this->m_recv_head_node->m_cur_len,
						this->m_data + copy_len, bytes_transferred);
					// ����ͷ���Ѵ�����
					this->m_recv_head_node->m_cur_len += bytes_transferred;
					// �������ݻ�����
					std::memset (this->m_data, 0, MAX_LENGTH);
					// �����첽�������ص�
					this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
						std::bind (&CSession::HandleRead, this,
							std::placeholders::_1, std::placeholders::_2, this->SharedSelf()));
					return;
				} // END if (bytes_transferred + this->m_recv_head_node->m_cur_len < HEAD_LENGTH)
				// �յ������ݱ�ͷ����ʱ
				// ͷ��ʣ��δ�����ĳ���
				int head_remain = HEAD_LENGTH - this->m_recv_head_node->m_cur_len;
				// ����ʣ��ͷ������
				std::memcpy (this->m_recv_head_node->m_data + this->m_recv_head_node->m_cur_len,
					this->m_data + copy_len, head_remain);
				// �����Ѵ���ĳ��Ⱥ�ʣ��δ����ĳ���
				copy_len += head_remain;
				bytes_transferred -= head_remain;
				// ��ȡͷ�����ݣ���Ϣ�ĳ��ȣ�
				UInt16 data_len = 0;
				std::memcpy (&data_len, this->m_recv_head_node->m_data, HEAD_LENGTH);
				// ��ʱ��ת�����ֽ���
				std::cout << "data_len is " << data_len << std::endl;
				// ...

				// ����ͷ�����ȷǷ�
				if (data_len > MAX_LENGTH)
				{
					std::cout << "invalid data length is " << data_len << std::endl;
					this->m_server->ClearSession (this->m_uuid);
					return;
				} // END if (data_len > MAX_LENGTH)
				// ͷ��������ɣ�����洢��Ϣ������
				this->m_recv_msg_node =
					std::make_shared<MsgNode> (data_len);

				// ����Ϣ����С��ͷ���涨���ȣ�����δ��ȫ
				if (bytes_transferred < data_len)
				{
					// �Ƚ��������ݷŵ����սڵ���
					std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len,
						this->m_data + copy_len, bytes_transferred);
					// �����Խ��յĳ���
					this->m_recv_msg_node->m_cur_len += bytes_transferred;
					// ���û�����
					std::memset (this->m_data, 0, MAX_LENGTH);
					// �����첽�ص�����
					this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
						std::bind (&CSession::HandleRead, this,
							std::placeholders::_1,std::placeholders::_2, this->SharedSelf()));
					// ͷ���������
					this->m_b_head_parse = true;
					return;
				}// END if (bytes_transferred < data_len)
				// ��Ϣ���ȷ���ͷ���涨
				std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len, 
					this->m_data + copy_len, data_len);
				this->m_recv_msg_node->m_total_len += data_len;
				copy_len += data_len;
				// ��������Ϣ���ȼ�ȥ�˴δ���ĳ��ȣ��Ա���滹�����ݿ�������
				bytes_transferred -= data_len;
				// ����Ϣ��������ֹ��
				this->m_recv_msg_node->m_data[this->m_recv_msg_node->m_total_len] = '\0';
				//���Խ���Ϣ��ӡ��ȥ
				std::cout << "receive data is " << this->m_recv_msg_node->m_data << std::endl;
				// ����Send���Ͳ���
				this->Send (this->m_recv_msg_node->m_data, this->m_recv_msg_node->m_total_len);
				// ������ѯʣ��δ���յ�����
				this->m_b_head_parse = false;
				this->m_recv_head_node->Clean ( );
				if (bytes_transferred <= 0)
				{
					// ���û�����
					std::memset (this->m_data, 0, MAX_LENGTH);
					// �����첽�ص�����
					this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
						std::bind (&CSession::HandleRead, this,
							std::placeholders::_1, std::placeholders::_2, this->SharedSelf ( )));
					return;

				} // END if (bytes_transferred <= 0)
				continue;
			} // END if (this->m_b_head_parse)

			// ͷ���Ѿ������꣬������Ϣ��û�д�����ʱ
			// ���ܵ���Ϣ�Բ���ʣ���δ����
			
			// ��ȡʣ��δ�������Ϣ�峤��
			int remain_msg =
				this->m_recv_msg_node->m_total_len - this->m_recv_msg_node->m_cur_len;
			if (bytes_transferred < remain_msg)
			{
				std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len,
					this->m_data + copy_len, bytes_transferred);
				// �����Ѵ���ĳ���
				this->m_recv_msg_node->m_cur_len += bytes_transferred;
				// ���û�����
				std::memset (this->m_data, 0, MAX_LENGTH);
				// �����첽�ص�����
				this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
					std::bind (&CSession::HandleRead, this,
						std::placeholders::_1, std::placeholders::_2, this->SharedSelf ( )));
				return;
			} // END if (bytes_transferred < remain_msg)

			
			std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len,
				this->m_data + copy_len, remain_msg);
			// �����Ѵ�����
			this->m_recv_msg_node->m_cur_len += remain_msg;
			// ���·��ͽ����ĳ���
			bytes_transferred -= remain_msg;
			// �����Կ����ĳ���
			copy_len += remain_msg;
			// ����Ϣ��������ֹ��
			this->m_recv_msg_node->m_data[this->m_recv_msg_node->m_total_len] = '\n';
			// ��ӡ����
			std::cout << "receive data is " << this->m_recv_msg_node->m_data << std::endl;
			// Send ����
			this->Send (this->m_recv_msg_node->m_data, this->m_recv_msg_node->m_total_len);
			// ������ѯʣ��δ���յ�����
			this->m_b_head_parse = false;
			this->m_recv_head_node->Clean ( );
			if (bytes_transferred <= 0)
			{
				// ���û�����
				std::memset (this->m_data, 0, MAX_LENGTH);
				// �����첽�ص�����
				this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
					std::bind (&CSession::HandleRead, this,
						std::placeholders::_1, std::placeholders::_2, this->SharedSelf ( )));
				return;

			} // END if (bytes_transferred <= 0)

			continue;
		} // END while (bytes_transferred > 0)
	} // END if (!err)
	else
	{
		std::clog << "handle read faild, error is " << err.what ( ) << std::endl;
		this->Close ( );
		this->m_server->ClearSession (this->m_uuid );
	} // END else
}

auto CSession::HandleWrite (const boost::system::error_code & err,
	std::shared_ptr<CSession> shared_self) 
	-> void
{
	// �ж��쳣
	if (!err)
	{
		// ���û�����
		std::lock_guard<std::mutex> lock (this->m_sed_lock);
		std::cout << "send data " << this->m_send_que.front ( )->m_data + HEAD_LENGTH << std::endl;
		this->m_send_que.pop ( );

		// ��������Ƿ�Ϊ�� 
		if (!this->m_send_que.empty ( ))
		{
			auto& msgnode = this->m_send_que.front ( );
			std::cout << "д�� �� " << msgnode->m_data << std::endl;
			asio::async_write (this->m_sock, asio::buffer (msgnode->m_data, msgnode->m_total_len),
				std::bind (&CSession::HandleWrite, this,
					std::placeholders::_1, this->SharedSelf ( )));
		} // END if (!this->m_send_que.empty ( ))
	} // END if (!err)
	else
	{
		std::cout << "handle write faild error is " << err.what ( ) << std::endl;
		this->Close ( );
		this->m_server->ClearSession (this->m_uuid);
	}

}




