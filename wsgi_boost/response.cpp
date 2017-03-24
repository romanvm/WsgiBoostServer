#include "response.h"

#include <boost/format.hpp>

using namespace std;
namespace sys = boost::system;


namespace wsgi_boost
{
	void Response::buffer_header(const string& status, headers_type& headers)
	{
		m_connection.buffer_output(http_version + " " + status + "\r\n");
		headers.emplace_back("Server", m_server_name);
		headers.emplace_back("Date", get_current_gmt_time());
		if (keep_alive)
			headers.emplace_back("Connection", "keep-alive");
		else
			headers.emplace_back("Connection", "close");
		for (const auto& header : headers)
			m_connection.buffer_output(header.first + ": " + header.second + "\r\n");
		m_connection.buffer_output("\r\n");
	}

	sys::error_code Response::send_header(const string& status, headers_type& headers, bool async)
	{
		buffer_header(status, headers);
		m_header_sent = true;
		return m_connection.flush(async);
	}


	sys::error_code Response::send_data(const string& data, bool async)
	{
		m_connection.buffer_output(data);
		return m_connection.flush(async);
	}


	sys::error_code Response::send_mesage(const string& status, const string& message, bool async)
	{
		headers_type headers;
		headers.emplace_back("Content-Length", to_string(message.length()));
		if (message.length() > 0)
			headers.emplace_back("Content-Type", "text/plain");
		sys::error_code ec = send_header(status, headers, async);
		if (!ec)
			ec = send_data(message, async);
		return ec;
	}


	sys::error_code Response::send_html(const string& status, const string& title,
		const string& header, const string& text, bool async)
	{
		boost::format tpl{ html_template };
		tpl % title % header % text;
		string html = tpl.str();
		headers_type headers;
		headers.emplace_back("Content-Type", "text/html");
		headers.emplace_back("Content-Length", to_string(html.length()));
		sys::error_code ec = send_header(status, headers, async);
		if (!ec)
			ec = send_data(html, async);
		return ec;
	}

	void Response::buffer_data(const std::string& data) { m_connection.buffer_output(data); }

	bool Response::header_sent() { return m_header_sent; }

	void Response::clear() { m_connection.clear_output(); }
}
