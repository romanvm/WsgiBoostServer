#include "connection.h"

#include <boost/system/system_error.hpp>

#include <sstream>
#include <vector>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;
namespace py = boost::python;

namespace wsgi_boost
{
#pragma region Connection

	void Connection::set_timeout(unsigned int timeout)
	{
		m_timer.expires_from_now(boost::posix_time::seconds(timeout));
		m_timer.async_wait(m_strand.wrap([this](const sys::error_code& ec)
		{
			if (!ec)
			{
				m_socket->shutdown(asio::ip::tcp::socket::shutdown_both);
				m_socket->close();
			}
		}));
	}


	sys::error_code Connection::read_header_line(string& line)
	{
		sys::error_code ec;
		set_timeout(m_header_timeout);
		size_t bytes_read = asio::read_until(*m_socket, m_istreambuf, "\r\n", ec);
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
		// For receivind POST content I'm using a syncronous read because a stackful coroutine can be resumed
		// in a different thread while the GIL is held which results in Python crash.
		size_t bytes_read = asio::read(*m_socket, m_istreambuf, asio::transfer_exactly(size), ec);
		if (!ec || (ec && bytes_read > 0))
		{
			m_timer.cancel();
			return true;
		}
		return false;
	}


	bool Connection::read_bytes(string& data, long long length)
	{
		bool result = read_into_buffer(length);
		if (result)
		{
			ostringstream oss;
			oss << &m_istreambuf;
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
			m_bytes_left -= temp.length();
			if (!is.eof())
			{
				line += '\n';
				--m_bytes_left;
				break;
			}
			if (read_into_buffer(min(long long(128), m_bytes_left)))
			{
				is.clear();
			}
			else
			{
				break;
			}
		}
		return line;
	}


	void Connection::set_post_content_length(long long cl)
	{
		m_bytes_left = cl;
		m_content_length = cl;
	}


	sys::error_code Connection::flush()
	{
		sys::error_code ec;
		set_timeout(m_content_timeout);
		// For sending HTTP response I'm using a syncronous write because a stackful coroutine can be resumed
		// in a different thread while the GIL is held which results in Python crash.
		asio::write(*m_socket, m_ostreambuf, ec);
		if (!ec)
			m_timer.cancel();
		return ec;
	}
#pragma endregion

#pragma region InputWrapper

	string InputWrapper::read(long long size)
	{
		string data;
		if (m_connection.read_bytes(data, size))
			return data;
		return "";
	}

	string InputWrapper::readline(long long size)
	{
		string line = m_connection.read_line();
		if (size > 0 && line.length() > size)
			line = line.substr(0, size);
		return line;
	}

	py::list InputWrapper::readlines(long long hint)
	{
		py::list listing;
		while (true)
		{
			string line = readline();
			if (line != "")
			{
				listing.append(line);
			}
			else
			{
				break;
			}
		}
		return listing;
	}

	InputWrapper* InputWrapper::iter()
	{
		return this;
	}

	std::string InputWrapper::next()
	{
		string line = readline();
		if (line != "")
		{
			return line;
		}
		else
		{
			throw StopIteration();
		}
	}

	long long InputWrapper::len()
	{
		return m_connection.content_length();
	}
#pragma endregion
}
