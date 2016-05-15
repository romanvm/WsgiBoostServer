#pragma once
/*
HTTP connection for receiving and sending data

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "exceptions.h"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/python.hpp>

#include<memory>
#include<iostream>

namespace wsgi_boost
{
	typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

	// Represents a http connection to a client
	class Connection : public std::ostream
	{
	private:
		socket_ptr m_socket;
		boost::asio::streambuf m_istreambuf;
		boost::asio::streambuf m_ostreambuf;
		boost::asio::deadline_timer m_timer;
		boost::asio::strand m_strand;
		unsigned int m_header_timeout;
		unsigned int m_content_timeout;
		long long m_bytes_left = -1;
		long long m_content_length = -1;

		void set_timeout(unsigned int timeout);

		bool read_into_buffer(long long length = -1);

	public:
		Connection(const Connection&) = delete;
		Connection& operator=(const Connection&) = delete;

		Connection(socket_ptr socket, boost::asio::io_service& io_service,
			unsigned int header_timeout, unsigned int content_timeout) :
			std::ostream(&m_ostreambuf),
			m_socket{ socket }, m_strand{ io_service }, m_timer{ io_service },
			m_header_timeout{ header_timeout }, m_content_timeout{ content_timeout } {}

		// Read header line
		boost::system::error_code read_header_line(std::string& line);

		// Read line including a new line charachter
		std::string read_line();

		// Read a specified number of bytes or all data left
		bool read_bytes(std::string& data, long long length = -1);

		// Set content length to control reading POST data
		void set_post_content_length(long long cl);

		// Send all output data to the client
		boost::system::error_code flush();

		// Get asio socket pointer
		std::shared_ptr<boost::asio::ip::tcp::socket> socket() const
		{
			return m_socket;
		}

		long long content_length() const
		{
			return m_content_length;
		}
	};


	class InputWrapper
	{
	private:
		Connection& m_connection;

	public:
		explicit InputWrapper(Connection& conn) : m_connection{ conn } {}

		std::string read(long long size = -1);

		std::string readline(long long size = -1);

		boost::python::list readlines(long long hint = -1);

		InputWrapper* iter();

		std::string next();

		long long len();
	};
}
