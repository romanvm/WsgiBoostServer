#include "request.h"

#include <boost/algorithm/string.hpp>

using namespace std;
namespace alg = boost::algorithm;
namespace sys = boost::system;


namespace wsgi_boost
{
	boost::system::error_code Request::parse_header()
	{
		string header;
		sys::error_code ec = m_connection.read_header(header);
		if (ec)
			return ec;
		istringstream iss{ header };
		string line;
		getline(iss, line);
		line.pop_back();
		vector<string> parts;
		alg::split(parts, line, alg::is_space(), alg::token_compress_on);
		if (parts.size() != 3)
			return sys::error_code(sys::errc::bad_message, sys::system_category());
		method = parts[0];
		path = parts[1];
		http_version = parts[2];
		while (true)
		{
			getline(iss, line);
			if (!iss.good())
				break;
			line.pop_back();
			size_t pos = line.find(':');
			if (pos != string::npos)
			{
				string header = alg::trim_copy(line.substr(0, pos));
				string value = alg::trim_copy(line.substr(pos + 1));
				if (headers.find(header) == headers.end())
				{
					headers.emplace(header, value);
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
				m_connection.set_post_content_length(-1);
			}
		}
		else
		{
			m_connection.set_post_content_length(-1);
		}
		if ((method == "POST" || method == "PUT") && m_connection.content_length() == -1)
			return sys::error_code(sys::errc::invalid_argument, sys::system_category());
		return sys::error_code(sys::errc::success, sys::system_category());
	}


	bool Request::check_header(const string& header, string value)
	{
		auto it = headers.find(header);
		if (it != headers.end())
		{
			string actual_value = it->second;
			alg::to_lower(value);
			alg::to_lower(actual_value);
			return actual_value.find(value) != string::npos;
		}
		else
		{
			return false;
		}
	}


	string Request::get_header(const string& header)
	{
		auto it = headers.find(header);
		if (it != headers.end())
			return it->second;
		return string();
	}

	bool Request::keep_alive()
	{
		return check_header("Connection", "keep-alive") || http_version == "HTTP/1.1";
	}


	Connection& Request::connection() const
	{
		return m_connection;
	}


	string Request::remote_address()
	{
		return m_connection.socket()->remote_endpoint().address().to_string();
	}


	unsigned short Request::remote_port()
	{
		return m_connection.socket()->remote_endpoint().port();
	}
}
