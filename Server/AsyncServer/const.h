#pragma once
#ifndef __CONST_H__
#define __CONST_H__
typedef unsigned int UInt32;
using UInt16 = unsigned short;

/* ��Ϣͷ������ */ constexpr unsigned short HEAD_LENGTH = 2;
/* ��Ϣ��󳤶� */ constexpr unsigned int MAX_LENGTH = 1024 * 2;
/* Ĭ�϶˿ں�*/ constexpr UInt16 port = 10086;


constexpr std::size_t MAX_RECVQUE = 10000;
constexpr UInt32 MAX_SENDQUE = 1000;




#endif // !__CONST_H__


