#include "response.h"

using namespace std;
namespace sys = boost::system;


sys::error_code Response::send_header(const string& status_code, const string& status_msg, headers_type& headers)
{
	m_connection << http_version << " " << status_code << " " << status_msg << "\r\n";
	headers.emplace_back("Server", m_server_name);
	if (http_version == "HTTP/1.1")
		headers.emplace_back("Connection", "keep-alive");
	for (const auto& header : headers)
	{
		m_connection << header.first << ": " << header.second << "\r\n";
	}
	m_connection << "\r\n";
	return m_connection.flush();
}


sys::error_code Response::send_data(const string& data)
{
	m_connection << data;
	return m_connection.flush();
}


sys::error_code Response::send_mesage(const string& status_code, const string& status_msg, const string& message)
{
	headers_type headers;
	if (message.length() > 0)
	{
		headers.emplace_back("Content-Type", "text/plain");
		headers.emplace_back("Content-Length", to_string(message.length()));
	}
	sys::error_code ec = send_header(status_code, status_msg, headers);
	if (!ec)
		ec = send_data(message);
	return ec;
}
