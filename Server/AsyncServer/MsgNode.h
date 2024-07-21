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
	//	����
	char* m_data;
	//	�����ܳ���
	short m_total_len;
	//	�����Ѵ�����
	short m_cur_len;
};

#endif // !__MSG_NODE_H__


