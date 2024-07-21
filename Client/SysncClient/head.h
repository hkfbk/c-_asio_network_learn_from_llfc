#pragma once
#ifndef __HEAD_H__
#define __HEAD_H__
#include <iostream>
#include "boost/asio.hpp"
#include <string>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace asio = boost::asio;
namespace ip = boost::asio::ip;
constexpr std::size_t MAX_LENGTH = 1024 * 2;
constexpr short HEAD_LENGTH = 2;



#endif // __HEAD_H__
