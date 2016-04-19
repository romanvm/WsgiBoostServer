#pragma once

#include "utils.h"

#include <iostream>


class BaseRequestHandler
{
protected:
	std::ostream& _response;
	const std::string& _server_name;
	const std::string& _http_version;

public:
	BaseRequestHandler(std::ostream& response, const std::string& server_name, const std::string& http_version):
		_response{response}, _server_name{server_name}, _http_version{http_version} {}
	virtual ~BaseRequestHandler() {}

	virtual void send_code(const std::string& code, const std::string message = "")
	{
		_response << "HTTP/" << _http_version << " " << code << "\r\n";
		_response << "Server: " << _server_name << "\r\n";
		_response << "Date: " << get_current_gmt_time() << "\r\n";
		_response << "Content-Length: " << message.length() << "\r\n";
		_response << "Content-Type: text/plain\r\nConnection: close\r\n\r\n" << message;
	}
};
