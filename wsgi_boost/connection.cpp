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
		m_timer.async_wait(m_strand->wrap([this](const sys::error_code& ec)
		{
			if (ec != asio::error::operation_aborted)
			{
				m_socket->shutdown(asio::ip::tcp::socket::shutdown_both);
				m_socket->close();
			}
		}));
	}


	sys::error_code Connection::read_header(string& header)
	{
		sys::error_code ec;
		set_timeout(m_header_timeout);
		size_t bytes_read = asio::async_read_until(*m_socket, m_istreambuf, "\r\n\r\n", m_yc[ec]);
		m_timer.cancel();
		if (!ec)
		{
			vector<char> buffer(bytes_read);
			istream is{ &m_istreambuf };
			is.read(&buffer[0], bytes_read);
			header = string(buffer.begin(), buffer.end() - 2);
		}
		return ec;
	}


	bool Connection::read_into_buffer(long long length, bool async)
	{
		if (m_bytes_left <= 0)
			return false;
		size_t residual_bytes = m_istreambuf.size();
		size_t size;
		if (length >= 0 && length > residual_bytes)
			size = min(m_bytes_left, length - (long long)residual_bytes);
		else if (length >= 0 && length <= residual_bytes)
			return true;
		else
			size = m_bytes_left - residual_bytes;
		sys::error_code ec;
		size_t bytes_read;
		set_timeout(m_content_timeout);
		if (async)
		{
			// Initial POST buffering is done without GIL so switching threads does not matter.
			// In single-threaded mode read operations are also does not asyncronously.
			bytes_read = asio::async_read(*m_socket, m_istreambuf, asio::transfer_exactly(size), m_yc[ec]);
		}
		else
		{
			// With multiple threads for receivind POST content I'm using a syncronous read
			// because a stackful coroutine can be resumed
			// in a different thread while the GIL is held which results in Python crash.
			bytes_read = asio::read(*m_socket, m_istreambuf, asio::transfer_exactly(size), ec);
		}
		m_timer.cancel();
		if (!ec || (ec && bytes_read > 0))			
			return true;
		return false;
	}


	bool Connection::read_bytes(string& data, long long length, bool async)
	{
		bool result = read_into_buffer(length, async);
		if (result)
		{
			istream is{ &m_istreambuf };
			long long size;
			if (length > 0)
				size = min(length, m_bytes_left);
			else
				size = m_bytes_left;
			vector<char> buffer(size);
			is.read(&buffer[0], size);
			data = string(buffer.begin(), buffer.end());
			m_bytes_left -= data.length();
		}
		return result;
	}


	string Connection::read_line(bool async)
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
			if (read_into_buffer(min(128LL, m_bytes_left), async))
				is.clear();
			else
				break;
		}
		return line;
	}


	void Connection::post_content_length(long long cl)
	{
		m_bytes_left = cl;
		m_content_length = cl;
	}


	sys::error_code Connection::flush(bool async)
	{
		sys::error_code ec;
		set_timeout(m_content_timeout);
		if (async)
		{
			// For sending static HTTP responses I'm using async write because static request handlers
			// operate without the GIL so switching threads does not matter.
			// With a singlie thread data is also sent to a client asyncronously.
			asio::async_write(*m_socket, m_ostreambuf, m_yc[ec]);
		}
		else
		{
			// With multiple threads for sending WSGI HTTP responses I'm using a syncronous write
			// because a stackful coroutine can be resumed
			// in a different thread while the GIL is held which results in Python crash.
			asio::write(*m_socket, m_ostreambuf, ec);
		}
		m_timer.cancel();
		return ec;
	}
#pragma endregion

#pragma region InputStream

	string InputStream::read(long long size)
	{
		GilRelease release_gil;
		string data;
		if (m_connection.read_bytes(data, size, m_async))
			return data;
		return "";
	}


	string InputStream::readline(long long size)
	{
		GilRelease release_gil;
		string line = m_connection.read_line(m_async);
		if (size > 0 && line.length() > size)
			line = line.substr(0, size);
		return line;
	}


	py::list InputStream::readlines(long long sizehint)
	{
		py::list listing;
		string line;
		long long total_length = 0;
		while ((line = readline()) != "")
		{
#if PY_MAJOR_VERSION < 3
			listing.append(line);
#else
			listing.append(get_py3_bytes(line));
#endif
			if (sizehint >= 0)
			{
				total_length += line.length();
				if (total_length > sizehint)
					break;
			}
			
		}
		return listing;
	}


	std::string InputStream::next()
	{
		string line = readline();
		if (line != "")
			return line;
		else
			throw StopIteration();
	}

#pragma endregion
}
