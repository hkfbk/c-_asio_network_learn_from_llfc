#include "MsgNode.h"

MsgNode::MsgNode (char* _data, short _length)
	:m_total_len (_length + HEAD_LENGTH), m_cur_len (0)
{
	m_data = new char[m_total_len + 1]();
	std::memcpy (m_data, &_length, HEAD_LENGTH);
	std::memcpy (m_data + HEAD_LENGTH, _data, _length);
	m_data[m_total_len] = '\0';

}

MsgNode::MsgNode (short _length)
	:m_total_len(_length), m_cur_len(0)
{
	m_data = new char[_length + 1];
	m_data[_length] = '\0';
}

auto MsgNode::Clean ( )
-> void
{
	std::memset (m_data, 0, m_total_len);
	m_cur_len = 0;
}
