#include "CSession.h"

CSession::CSession (asio::io_context& _ioc, CServer* _server)
	:m_sock(_ioc),m_server(_server),m_b_close(false),m_b_head_parse(false)
{ 
	//std::memcpy (m_data, 0, MAX_LENGTH);


	//	生成uuid
	boost::uuids::uuid uuid =
		boost::uuids::random_generator ( )();
	m_uuid = boost::uuids::to_string (uuid);

	//	存储消息节点
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
	// TODO: 在此处插入 return 语句
	return this->m_sock;
}

auto CSession::GetUuid ( ) -> std::string
{
	return m_uuid;
}

auto CSession::Send (char* _msg, int _max_length) -> void
{
	// 加锁
	std::lock_guard<std::mutex> lock (this->m_sed_lock);
	// 队列长度
	auto send_que_size = this->m_send_que.size ( );
	// 判断是否超过队列最大长度
	if (!send_que_size > MAX_SENDQUE)
	{
		std::cout << "session: " << this->m_uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	} // END if (!send_que_size > MAX_SENDQUE)

	// 将发送对象放入发送队列
	this->m_send_que.push (std::make_shared<MsgNode> (_msg, _max_length));
	// 检查队列是否为空
	if (send_que_size > 0)
		return;
	// 获取要发送的消息节点
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
	if (!err)//	检查是否有异常，如果有打印错误日志并关闭链接，
	{
		int copy_len = 0; // 保存已拷贝的长度
		while (bytes_transferred > 0)
		{
			//	检查是不是头部信息
			if (!this->m_b_head_parse)
			{
				// 先处理收到的数据不足头部数据大小的情况
				// 先取出这一部分，然后算出头部剩余多少，保存已处理长度， 继续read数据
				if (bytes_transferred + this->m_recv_head_node->m_cur_len < HEAD_LENGTH)
				{
					// 拷贝头部数据
					std::memcpy (this->m_recv_head_node->m_data + this->m_recv_head_node->m_cur_len,
						this->m_data + copy_len, bytes_transferred);
					// 更新头部已处理长度
					this->m_recv_head_node->m_cur_len += bytes_transferred;
					// 重置数据缓冲区
					std::memset (this->m_data, 0, MAX_LENGTH);
					// 设置异步读监听回调
					this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
						std::bind (&CSession::HandleRead, this,
							std::placeholders::_1, std::placeholders::_2, this->SharedSelf()));
					return;
				} // END if (bytes_transferred + this->m_recv_head_node->m_cur_len < HEAD_LENGTH)
				// 收到的数据比头部多时
				// 头部剩余未拷贝的长度
				int head_remain = HEAD_LENGTH - this->m_recv_head_node->m_cur_len;
				// 拷贝剩余头部数据
				std::memcpy (this->m_recv_head_node->m_data + this->m_recv_head_node->m_cur_len,
					this->m_data + copy_len, head_remain);
				// 更新已处理的长度和剩余未处理的长度
				copy_len += head_remain;
				bytes_transferred -= head_remain;
				// 获取头部数据（消息的长度）
				UInt16 data_len = 0;
				std::memcpy (&data_len, this->m_recv_head_node->m_data, HEAD_LENGTH);
				// 暂时不转网络字节序
				std::cout << "data_len is " << data_len << std::endl;
				// ...

				// 处理头部长度非法
				if (data_len > MAX_LENGTH)
				{
					std::cout << "invalid data length is " << data_len << std::endl;
					this->m_server->ClearSession (this->m_uuid);
					return;
				} // END if (data_len > MAX_LENGTH)
				// 头部处理完成，构造存储消息的容器
				this->m_recv_msg_node =
					std::make_shared<MsgNode> (data_len);

				// 当消息长度小于头部规定长度，数据未收全
				if (bytes_transferred < data_len)
				{
					// 先将部分数据放到接收节点里
					std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len,
						this->m_data + copy_len, bytes_transferred);
					// 保存以接收的长度
					this->m_recv_msg_node->m_cur_len += bytes_transferred;
					// 重置缓冲区
					std::memset (this->m_data, 0, MAX_LENGTH);
					// 设置异步回调监听
					this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
						std::bind (&CSession::HandleRead, this,
							std::placeholders::_1,std::placeholders::_2, this->SharedSelf()));
					// 头部处理完成
					this->m_b_head_parse = true;
					return;
				}// END if (bytes_transferred < data_len)
				// 消息长度符合头部规定
				std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len, 
					this->m_data + copy_len, data_len);
				this->m_recv_msg_node->m_total_len += data_len;
				copy_len += data_len;
				// 将接收消息长度减去此次处理的长度，以便后面还有数据可以再用
				bytes_transferred -= data_len;
				// 给消息体设置终止符
				this->m_recv_msg_node->m_data[this->m_recv_msg_node->m_total_len] = '\0';
				//尝试将消息打印出去
				std::cout << "receive data is " << this->m_recv_msg_node->m_data << std::endl;
				// 调用Send发送测试
				this->Send (this->m_recv_msg_node->m_data, this->m_recv_msg_node->m_total_len);
				// 继续轮询剩余未接收的数据
				this->m_b_head_parse = false;
				this->m_recv_head_node->Clean ( );
				if (bytes_transferred <= 0)
				{
					// 重置缓冲区
					std::memset (this->m_data, 0, MAX_LENGTH);
					// 设置异步回调监听
					this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
						std::bind (&CSession::HandleRead, this,
							std::placeholders::_1, std::placeholders::_2, this->SharedSelf ( )));
					return;

				} // END if (bytes_transferred <= 0)
				continue;
			} // END if (this->m_b_head_parse)

			// 头部已经处理完，但是消息体没有处理完时
			// 接受的消息仍不足剩余的未处理
			
			// 获取剩余未处理的消息体长度
			int remain_msg =
				this->m_recv_msg_node->m_total_len - this->m_recv_msg_node->m_cur_len;
			if (bytes_transferred < remain_msg)
			{
				std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len,
					this->m_data + copy_len, bytes_transferred);
				// 更新已处理的长度
				this->m_recv_msg_node->m_cur_len += bytes_transferred;
				// 重置缓冲区
				std::memset (this->m_data, 0, MAX_LENGTH);
				// 设置异步回调监听
				this->m_sock.async_read_some (asio::buffer (this->m_data, MAX_LENGTH),
					std::bind (&CSession::HandleRead, this,
						std::placeholders::_1, std::placeholders::_2, this->SharedSelf ( )));
				return;
			} // END if (bytes_transferred < remain_msg)

			
			std::memcpy (this->m_recv_msg_node->m_data + this->m_recv_msg_node->m_cur_len,
				this->m_data + copy_len, remain_msg);
			// 更新已处理长度
			this->m_recv_msg_node->m_cur_len += remain_msg;
			// 更新发送进来的长度
			bytes_transferred -= remain_msg;
			// 更新以拷贝的长度
			copy_len += remain_msg;
			// 给消息体设置终止符
			this->m_recv_msg_node->m_data[this->m_recv_msg_node->m_total_len] = '\n';
			// 打印测试
			std::cout << "receive data is " << this->m_recv_msg_node->m_data << std::endl;
			// Send 测试
			this->Send (this->m_recv_msg_node->m_data, this->m_recv_msg_node->m_total_len);
			// 继续轮询剩余未接收的数据
			this->m_b_head_parse = false;
			this->m_recv_head_node->Clean ( );
			if (bytes_transferred <= 0)
			{
				// 重置缓冲区
				std::memset (this->m_data, 0, MAX_LENGTH);
				// 设置异步回调监听
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
	// 判断异常
	if (!err)
	{
		// 设置互斥锁
		std::lock_guard<std::mutex> lock (this->m_sed_lock);
		std::cout << "send data " << this->m_send_que.front ( )->m_data + HEAD_LENGTH << std::endl;
		this->m_send_que.pop ( );

		// 检查容器是否为空 
		if (!this->m_send_que.empty ( ))
		{
			auto& msgnode = this->m_send_que.front ( );
			std::cout << "写入 ： " << msgnode->m_data << std::endl;
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




