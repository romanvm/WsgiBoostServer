#pragma once
/*
HTTP connection for receiving and sending data

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "exceptions.h"
#include "utils.h"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/python.hpp>

#include <memory>
#include <iostream>
#include <string>

namespace wsgi_boost
{
	typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

	// Represents a http connection to a client
	class Connection
	{
	private:
		socket_ptr m_socket;
		boost::asio::streambuf m_istreambuf;
		boost::asio::streambuf m_ostreambuf;
		boost::asio::deadline_timer m_timer;
		unsigned int m_header_timeout;
		unsigned int m_content_timeout;
		long long m_bytes_left = -1;
		long long m_content_length = -1;
		boost::asio::yield_context m_yc;

		void set_timeout(unsigned int timeout);

	public:
		Connection(const Connection&) = delete;
		Connection& operator=(const Connection&) = delete;

		Connection(socket_ptr socket, std::shared_ptr<boost::asio::io_service> io_service,
			boost::asio::yield_context yc, unsigned int header_timeout, unsigned int content_timeout) :
			m_socket{ socket }, m_timer{ *io_service }, m_yc{ yc },
			m_header_timeout{ header_timeout }, m_content_timeout{ content_timeout } {}

		// Read HTTP header
		boost::system::error_code read_header(std::string& header);

		// Read data from a socket into the input buffer
		bool read_into_buffer(long long length = -1, bool async = false);

		// Read line including a new line charachter
		std::string read_line();

		// Read a specified number of bytes or all data left
		bool read_bytes(std::string& data, long long length = -1);

		// Set content length to control reading POST data
		void post_content_length(long long cl);

		// Get POST content length
		long long post_content_length() const;

		// Save data to the output buffer
		void buffer_output(const std::string& data);

		// Send all output data to the client
		boost::system::error_code flush(bool async = true);

		// Get asio socket pointer
		socket_ptr socket() const;
	};

	// Wraps Connection instance to provide Python file-like object for wsgi.input
	class InputStream
	{
	private:
		Connection& m_connection;

	public:
		explicit InputStream(Connection& conn) : m_connection{ conn } {}

		// Read data from input content
		std::string read(long long size = -1);

		// Read a line from input content
		std::string readline(long long size = -1);

		// Read a list of lines from input content
		boost::python::list readlines(long long sizehint = -1);

		// Return Python iterator
		InputStream* iter() { return this; }

		// Iterate the iterator
		std::string next();

#if PY_MAJOR_VERSION >= 3
		// In Python3 wsgi.input must return bytes

		// Read data from input content
		boost::python::object read_bytes(long long size = -1);

		// Read a line from input content
		boost::python::object read_byte_line(long long size = -1);

		// Iterate the iterator
		boost::python::object next_bytes();

	private:
		// Convert C++ string to Python 3 bytes object
		boost::python::object string_to_bytes(const std::string& str) const;
#endif
	};
}
