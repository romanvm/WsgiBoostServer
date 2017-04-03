#include "request.h"

#include <boost/algorithm/string.hpp>

using namespace std;
namespace alg = boost::algorithm;
namespace sys = boost::system;


namespace wsgi_boost
{
	parse_result Request::parse_header()
	{
		string header;
		sys::error_code ec = m_connection.read_header(header);
		if (ec)
			return CONN_ERROR;
		istringstream iss{ header };
		string line;
		getline(iss, line);
		line.pop_back();
		vector<string> parts;
		alg::split(parts, line, alg::is_space(), alg::token_compress_on);
		if (parts.size() != 3)
			return BAD_REQUEST;
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
				string name = alg::trim_copy(line.substr(0, pos));
				string value = alg::trim_copy(line.substr(pos + 1));
				if (headers.find(name) == headers.end())
					headers.emplace(name, value);
				else
					headers[name] += ", " + value;
			}
		}
		if (method == "POST" || method == "PUT" || method == "PATCH")
		{
			try
			{
				long long cl = stoll(get_header("Content-Length"));
				m_connection.post_content_length(cl);
			}
			catch (const logic_error&)
			{
				return LENGTH_REQUIRED;
			}
		}
		else
		{
			m_connection.post_content_length(-1);
		}
		return PARSE_OK;
	}


	bool Request::check_header(const string& header, const string& value) const
	{
		auto it = headers.find(header);
		return (it != headers.end()) && boost::algorithm::icontains(it->second, value);
	}


	string Request::get_header(const string& header) const
	{
		auto it = headers.find(header);
		return it != headers.end() ? it->second : string();
	}

	bool Request::keep_alive() const
	{
		return check_header("Connection", "keep-alive") || (http_version == "HTTP/1.1" && !check_header("Connection", "close"));
	}


	Connection& Request::connection() const
	{
		return m_connection;
	}


	string Request::remote_address() const
	{
		return m_connection.socket()->lowest_layer().remote_endpoint().address().to_string();
	}


	unsigned short Request::remote_port() const
	{
		return m_connection.socket()->lowest_layer().remote_endpoint().port();
	}
}
