#pragma once
/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "utils.h"

#include <boost/regex.hpp>
#include <boost/python.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <unordered_map>


namespace WsgiBoost
{
	class BaseRequestHandler
	{
	protected:
		std::ostream& _response;
		const std::string& _server_name;
		const std::string& _http_version;
		std::vector<std::pair<std::string, std::string>> _out_headers;

	public:
		BaseRequestHandler(std::ostream& response, const std::string& server_name, const std::string& http_version) :
			_response{ response },
			_server_name{ server_name },
			_http_version{ http_version }
		{
			_initialize_headers();
		}

		virtual ~BaseRequestHandler(){}

		BaseRequestHandler(const BaseRequestHandler&) = delete;
		BaseRequestHandler(BaseRequestHandler&&) = delete;

		virtual void handle_request() = 0;

		void send_code(const std::string& code, const std::string message = "")
		{
			_out_headers.emplace_back("Connection", "close");
			_send_string(code, message);
		}

	protected:
		void _initialize_headers()
		{
			_out_headers.emplace_back("Server", _server_name);
			_out_headers.emplace_back("Date", get_current_gmt_time());
		}

		void _send_http_header(const std::string& code)
		{
			_response << "HTTP/" << _http_version << " " << code << "\r\n";
			for (const auto& header : _out_headers)
			{
				_response << header.first << ": " << header.second << "\r\n";
			}
			_response << "\r\n";
		}

		void _send_string(const std::string& code, const std::string& content_string = "")
		{
			_out_headers.emplace_back("Connection", "close");
			_out_headers.emplace_back("Content-Length", std::to_string(content_string.length()));
			if (content_string != "")
			{
				_out_headers.emplace_back("Content-Type", "text/plain");
			}
			_send_http_header(code);
			_response << content_string;
		}
	};


	class StaticRequestHandler : public BaseRequestHandler
	{
	private:
		const std::string& _method;
		const std::string& _path;
		const std::string& _content_dir;
		const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& _in_headers;
		const boost::regex& _path_regex;

	public:
		StaticRequestHandler(
				std::ostream& response,
				const std::string& server_name,
				const std::string& http_version,
				const std::string& method,
				const std::string& path,
				const std::string& content_dir,
				const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers,
				const boost::regex& path_regex
			) : BaseRequestHandler(response, server_name, http_version),
			_method{ method },
			_path{ path },
			_content_dir{ content_dir },
			_in_headers{ in_headers },
			_path_regex{ path_regex }
			{}

		void handle_request()
		{
			if (_method != "GET" && _method != "HEAD")
			{
				std::string code = "405 Method Not Allowed";
				_send_string(code, code);
				return;
			}
		}
	};

	
	class WsgiRequestHandler : public BaseRequestHandler
	{
	private:
		const std::string& _method;
		const std::string& _path;
		const std::string& _remote_endpoint_address;
		const unsigned short& _remote_endpoint_port;
		const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& _in_headers;
		std::istream& _in_content;
		boost::python::object& _app;	

	public:
		WsgiRequestHandler(
				std::ostream& response,
				const std::string& server_name,
				const std::string& http_version,
				const std::string& method,
				const std::string& path,
				const std::string& remote_endpoint_address,
				const unsigned short& remote_endpoint_port,
				const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers,
				std::istream& in_content,
				boost::python::object& app
			) : BaseRequestHandler(response, server_name, http_version),
			_method{ method },
			_path{ path },
			_remote_endpoint_address{ remote_endpoint_address },
			_remote_endpoint_port{ remote_endpoint_port },
			_in_headers{ in_headers },
			_in_content{ in_content },
			_app{ app }
			{}

		void handle_request()
		{
			std::string code = "501 Not Implemented";
			send_code(code, code);
		}
	};
}
