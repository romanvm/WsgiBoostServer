#pragma once
/*
HTTP response

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "connection.h"
#include "utils.h"

#include <boost/system/error_code.hpp>

#include <vector>
#include <string>


namespace wsgi_boost
{
	typedef std::vector<std::pair<std::string, std::string>> headers_type;

	class Response
	{
	private:
		const std::string m_server_name = "WsgiBoost v." WSGI_BOOST_VERSION;
		Connection& m_connection;
		bool m_header_sent = false;

	public:
		std::string http_version = "HTTP/1.1";
		bool keep_alive;

		Response(const Response&) = delete;
		Response& operator=(const Response&) = delete;

		explicit Response(Connection& connection) : m_connection{ connection } {}

		// Send HTTP header (status code + headers)
		boost::system::error_code send_header(const std::string& status, headers_type& headers, bool async = true);

		// Send data to the client
		boost::system::error_code send_data(const std::string& data, bool async = true);

		// Send a plain text HTTP message to a client
		boost::system::error_code send_mesage(const std::string& status, const std::string& message = std::string());

		// Send a html HTTP message to a client
		boost::system::error_code send_html(const std::string& status, const std::string& title,
			const std::string& header, const std::string& text);

		// Check if HTTP header has been sent
		bool header_sent() const;
	};
}
