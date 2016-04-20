#pragma once
/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "utils.h"

#include <iostream>


namespace WsgiBoost
{
	class BaseRequestHandler
	{
	protected:
		std::ostream& _response;
		const std::string& _server_name;
		const std::string& _http_version;

	public:
		BaseRequestHandler(std::ostream& response, const std::string& server_name, const std::string& http_version) :
			_response{ response }, _server_name{ server_name }, _http_version{ http_version }
		{}
		virtual ~BaseRequestHandler(){}

		virtual void handle_request() = 0;

		void send_code(const std::string& code, const std::string message = "")
		{
			_response << "HTTP/" << _http_version << " " << code << "\r\n";
			_response << "Server: " << _server_name << "\r\n";
			_response << "Date: " << get_current_gmt_time() << "\r\n";
			_response << "Content-Length: " << message.length() << "\r\n";
			_response << "Connection: close\r\n";
			if (message != "")
			{
				_response << "Content-Type: text/plain\r\n\r\n" << message;
			}
			else
			{
				_response << "\r\n";
			}
		}
	};
}
