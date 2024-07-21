#pragma once
#ifndef __MSG_NODE_H__
#define __MSG_NODE_H__
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "const.h"

class MsgNode
{
	friend class CSession;
public:
	MsgNode (char* _data, short _length);
	MsgNode (short _length);

	auto Clean ( ) -> void;

	~MsgNode ( )
	{
		delete[] m_data;
	}
private:
	//	数据
	char* m_data;
	//	数据总长度
	short m_total_len;
	//	数据已处理长度
	short m_cur_len;
};

#endif // !__MSG_NODE_H__


