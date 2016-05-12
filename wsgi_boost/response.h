#pragma once
/*
HTTP response

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "connection.h"

#include <boost/system/error_code.hpp>

#include <vector>
#include <string>


namespace wsgi_boost
{
	typedef std::vector<std::pair<std::string, std::string>> headers_type;

	class Response
	{
	private:
		Connection& m_connection;

	public:
		std::string http_version = "HTTP/1.1";

		Response(const Response&) = delete;
		Response& operator=(const Response&) = delete;

		explicit Response(Connection& connection) : m_connection{ connection } {}

		boost::system::error_code send_header(const std::string& status, headers_type& headers);

		boost::system::error_code send_data(const std::string& data);

		boost::system::error_code send_mesage(const std::string& status, const std::string& message = std::string());
	};
}
