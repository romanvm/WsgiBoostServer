#pragma once

#include "request.h"
#include "response.h"

#include <boost/asio.hpp>

#include <thread>


class HttpServer
{
private:
	boost::asio::io_service m_io_service;
	boost::asio::ip::tcp::acceptor m_acceptor;
	std::vector<std::thread> m_threads;
	unsigned int m_num_threads;
	std::string m_ip_address;
	unsigned short m_port;
	boost::asio::signal_set m_signals;

	void accept();
	void process_request(socket_ptr socket);
	boost::system::error_code send_response(Request& request, Response& response);

public:
	unsigned int header_timeout = 5;
	unsigned int content_timeout = 300;
	bool reuse_address = true;

	HttpServer(const HttpServer&) = delete;
	HttpServer& operator=(const HttpServer&) = delete;

	HttpServer(std::string ip_address = "", unsigned short port = 8000, unsigned int num_threads = 1);

	void start();
	void stop();
};
