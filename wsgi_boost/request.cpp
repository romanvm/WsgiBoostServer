#include "request.h"

#include <boost/algorithm/string.hpp>

using namespace std;
namespace alg = boost::algorithm;
namespace sys = boost::system;


namespace wsgi_boost
{
	boost::system::error_code Request::parse_header()
	{
		string line;
		sys::error_code ec = m_connection.read_header_line(line);
		if (ec)
			return ec;
		vector<string> parts;
		alg::split(parts, line, alg::is_space(), alg::token_compress_on);
		if (parts.size() != 3)
			return sys::error_code(sys::errc::bad_message, sys::system_category());
		method = parts[0];
		path = parts[1];
		http_version = parts[2];
		while (true)
		{
			ec = m_connection.read_header_line(line);
			if (ec)
				return ec;
			if (!line.length())
				break;
			size_t pos = line.find(':');
			if (pos != string::npos)
			{
				string header = alg::trim_copy(line.substr(0, pos));
				string value = alg::trim_copy(line.substr(pos + 1));
				alg::to_lower(value);
				if (headers.find(header) == headers.end())
				{
					headers[header] = value;
				}
				else
				{
					headers[header] += "," + value;
				}
			}
		}
		string cl_header = get_header("Content-Length");
		if (cl_header != "")
		{
			try
			{
				long long cl = stoll(cl_header);
				m_connection.set_post_content_length(cl);
			}
			catch (const exception&)
			{
				m_connection.set_post_content_length(0);
			}
		}
		else
		{
			m_connection.set_post_content_length(0);
		}
		return sys::error_code(sys::errc::success, sys::system_category());
	}


	bool Request::check_header(const string& header, string value)
	{
		alg::to_lower(value);
		auto it = headers.find(header);
		return it != headers.end() && it->second.find(value) != string::npos;
	}


	string Request::get_header(const string& header)
	{
		auto it = headers.find(header);
		if (it != headers.end())
			return it->second;
		return string();
	}


	void Request::read_remote_endpoint_data()
	{
		try
		{
			socket_ptr socket = m_connection.socket();
			m_remote_address = socket->remote_endpoint().address().to_string();
			m_remote_port = socket->remote_endpoint().port();
		}
		catch (const exception&) {}
	}
}
