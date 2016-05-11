#pragma once

#include "connection.h"

#include <boost/system/error_code.hpp>

#include <vector>
#include <string>

typedef std::vector<std::pair<std::string, std::string>> headers_type;


class Response
{
private:
	Connection& m_connection;
	const std::string m_server_name = "asio_http Server";

public:
	std::string http_version = "HTTP/1.1";

	Response(const Response&) = delete;
	Response& operator=(const Response&) = delete;

	explicit Response(Connection& connection) : m_connection{ connection } {}

	boost::system::error_code send_header(const std::string& status_code, const std::string& status_msg, headers_type& headers);

	boost::system::error_code send_data(const std::string& data);

	boost::system::error_code send_mesage(const std::string& status_code, const std::string& status_msg, const std::string& message = std::string());
};
