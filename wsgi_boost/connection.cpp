#include "connection.h"

#include <boost/system/system_error.hpp>

#include <sstream>
#include <vector>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;


void Connection::set_timeout(unsigned int timeout)
{
	m_timer.expires_from_now(boost::posix_time::seconds(timeout));
	m_timer.async_wait([this](const sys::error_code& ec)
	{
		if (!ec)
		{
			m_socket->shutdown(asio::ip::tcp::socket::shutdown_both);
			m_socket->close();
		}
	});
}


sys::error_code Connection::read_header_line(string& line)
{
	sys::error_code ec;
	set_timeout(m_header_timeout);
	size_t bytes_read = asio::async_read_until(*m_socket, m_istreambuf, "\r\n", m_yc[ec]);
	if (!ec)
	{
		m_timer.cancel();
		vector<char> buffer(bytes_read);
		istream is{ &m_istreambuf };
		is.read(&buffer[0], bytes_read);
		line = string(buffer.begin(), buffer.end() - 2);
	}
	return ec;
}


bool Connection::read_into_buffer(long long length)
{
	if (m_bytes_left <= 0)
		return false;
	size_t residual_bytes = m_istreambuf.size();
	size_t size;
	if (length >= 0 && length > residual_bytes)
	{
		size = min(m_bytes_left, length - residual_bytes);
	}
	else if (length >= 0 && length <= residual_bytes)
	{
		size = 0;
	}
	else
	{
		size = m_bytes_left - residual_bytes;
	}
	sys::error_code ec;
	set_timeout(m_content_timeout);
	size_t bytes_read = asio::async_read(*m_socket, m_istreambuf, asio::transfer_exactly(size), m_yc[ec]);
	if (!ec || ec && bytes_read > 0)
	{
		m_timer.cancel();
		m_bytes_left -= bytes_read;
		return true;
	}
	return false;
}


bool Connection::read_bytes(string& data, long long length)
{
	bool result = read_into_buffer(length);
	if (result)
	{
		istream is{ &m_istreambuf };
		ostringstream oss;
		oss << is.rdbuf();
		data = oss.str();
		m_bytes_left -= oss.str().length();
	}
	return result;
}


string Connection::read_line()
{
	istream is{ &m_istreambuf };
	string line = string();
	while (true)
	{
		string temp = string();
		getline(is, temp);
		line += temp;
		if (!is.eof())
		{
			line += '\n';
			break;
		}
		else
		{
			if (read_into_buffer(128))
			{
				is.clear();
			}
			else
			{
				break;
			}
		}
	}
	return line;
}


void Connection::set_post_content_length(long long cl)
{
	m_bytes_left = cl;
}


sys::error_code Connection::flush()
{
	sys::error_code ec;
	set_timeout(m_content_timeout);
	asio::async_write(*m_socket, m_ostreambuf, m_yc[ec]);
	if (!ec)
		m_timer.cancel();
	return ec;
}
