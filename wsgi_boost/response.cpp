#include "response.h"

using namespace std;
namespace sys = boost::system;


namespace wsgi_boost
{
	sys::error_code Response::send_header(const string& status, headers_type& headers, bool async)
	{
		m_connection << http_version << " " << status << "\r\n";
		headers.emplace_back("Server", m_server_name);
		headers.emplace_back("Date", get_current_gmt_time());
		if (keep_alive)
		{
			headers.emplace_back("Connection", "keep-alive");
		}
		else
		{
			headers.emplace_back("Connection", "close");
		}
		for (const auto& header : headers)
		{
			m_connection << header.first << ": " << header.second << "\r\n";
		}
		m_connection << "\r\n";
		return m_connection.flush(async);
	}


	sys::error_code Response::send_data(const string& data, bool async)
	{
		m_connection << data;
		return m_connection.flush(async);
	}


	sys::error_code Response::send_mesage(const string& status, const string& message)
	{
		headers_type headers;
		headers.emplace_back("Content-Length", to_string(message.length()));
		if (message.length() > 0)
		{
			headers.emplace_back("Content-Type", "text/plain");
		}
		sys::error_code ec = send_header(status, headers);
		if (!ec)
			ec = send_data(message);
		return ec;
	}
}
