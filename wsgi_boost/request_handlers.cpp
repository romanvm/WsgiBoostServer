/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "request_handlers.h"

using namespace std;
namespace py = boost::python;
namespace fs = boost::filesystem;

namespace wsgi_boost
{
#pragma region BaseRequestHandler

	BaseRequestHandler::BaseRequestHandler(
			ostream& response,
			const string& server_name,
			const string& http_version,
			const string& method,
			const string& path,
			const unordered_multimap<string, string, ihash, iequal_to>& in_headers) :
		m_response{ response }, m_server_name{ server_name }, m_http_version{ http_version },
		m_in_headers{ in_headers }, m_method{ method }, m_path{ path }
	{
		initialize_headers();
	}

	void BaseRequestHandler::send_status(const string message)
	{
		out_headers.emplace_back("Content-Length", to_string(message.length()));
		if (message != "")
		{
			out_headers.emplace_back("Content-Type", "text/plain");
		}
		send_http_header();
		m_response << message;
	}

	void BaseRequestHandler::send_http_header()
	{
		m_response << "HTTP/" << m_http_version << " " << status << "\r\n";
		for (const auto& header : out_headers)
		{
			m_response << header.first << ": " << header.second << "\r\n";
		}
		m_response << "\r\n";
	}

	void BaseRequestHandler::initialize_headers()
	{
		out_headers.emplace_back("Server", m_server_name);
		out_headers.emplace_back("Date", get_current_gmt_time());
		const auto conn_iter = m_in_headers.find("Connection");
		if (conn_iter != m_in_headers.end())
		{
			out_headers.emplace_back("Connection", conn_iter->second);
		}
	}

#pragma endregion
#pragma region StaticRequestHandler



#pragma endregion


}
