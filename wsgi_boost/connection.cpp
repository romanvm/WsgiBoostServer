#include "connection.h"

#include <boost/system/system_error.hpp>

#include <sstream>
#include <vector>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;
namespace py = pybind11;


namespace wsgi_boost
{
#pragma region Connection

	void Connection::set_timeout(unsigned int timeout)
	{
		m_timer.expires_from_now(boost::posix_time::seconds(timeout));
		m_timer.async_wait([this](const sys::error_code& ec)
		{
			if (ec != asio::error::operation_aborted)
			{
				m_socket->lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
				m_socket->lowest_layer().close();
			}
		});
	}


	sys::error_code Connection::read_header(string& header)
	{
		sys::error_code ec;
		set_timeout(m_header_timeout);
		size_t bytes_read = asio::async_read_until(*m_socket, m_istreambuf, "\r\n\r\n", m_yc[ec]);
		m_timer.cancel();
		if (!ec)
		{
			auto in_buffer = m_istreambuf.data();
			auto buffers_begin = asio::buffers_begin(in_buffer);
			header.assign(buffers_begin, buffers_begin + bytes_read);
			m_istreambuf.consume(bytes_read);
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
			bytes_read = asio::async_read(*m_socket, m_istreambuf, asio::transfer_exactly(size), m_yc[ec]);
		else
			// Read/write operations executed from inside Python must be syncronous!
			bytes_read = asio::read(*m_socket, m_istreambuf, asio::transfer_exactly(size), ec);
		m_timer.cancel();
		if (!ec || (ec && bytes_read > 0))			
			return true;
		return false;
	}


	bool Connection::read_bytes(string& data, long long length)
	{
		bool result = read_into_buffer(length);
		if (result)
		{
			long long size;
			if (length > 0)
				size = min(length, m_bytes_left);
			else
				size = m_bytes_left;
			auto in_buffer = m_istreambuf.data();
			auto buffers_begin = asio::buffers_begin(in_buffer);
			data.assign(buffers_begin, buffers_begin + size);
			m_istreambuf.consume(size);
			m_bytes_left -= size;
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
			if (read_into_buffer(min(128LL, m_bytes_left)))
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


	long long Connection::post_content_length() const { return m_content_length; }


	void Connection::buffer_output(const string& data)
	{
		size_t length = data.length();
		auto out_buffers = m_ostreambuf.prepare(length);
		auto buffers_begin = asio::buffers_begin(out_buffers);
		copy(data.begin(), data.end(), buffers_begin);
		m_ostreambuf.commit(length);
	}


	sys::error_code Connection::flush(bool async)
	{
		sys::error_code ec;
		set_timeout(m_content_timeout);
		if (async)
			asio::async_write(*m_socket, m_ostreambuf, m_yc[ec]);
		else
			// Read/write operations executed from inside Python must be syncronous!
			asio::write(*m_socket, m_ostreambuf, ec);
		m_timer.cancel();
		return ec;
	}

	socket_ptr Connection::socket() const { return m_socket; }
#pragma endregion

#pragma region InputStream

	string InputStream::read_(long long size)
	{
		py::gil_scoped_release release_gil;
		string data;
		if (m_connection.read_bytes(data, size))
			return data;
		return string();
	}


	py::bytes InputStream::read(long long size)
	{
		return read_(size);
	}


	string InputStream::readline_()
	{
		py::gil_scoped_release release_gil;
		return m_connection.read_line();
	}


	py::bytes InputStream::readline(long long size)
	{
		// size argument is ignored
		return readline_();
	}


	py::list InputStream::readlines(long long sizehint)
	{
		py::list listing;
		string line;
		long long total_length = 0;
		while (!(line = readline()).empty())
		{
			listing.append(py::bytes(line));
			if (sizehint >= 0)
			{
				total_length += line.length();
				if (total_length > sizehint)
					break;
			}
			
		}
		return listing;
	}


	py::bytes InputStream::next()
	{
		string line = readline_();
		if (line.empty())
			throw py::stop_iteration();
		return line;
	}
#pragma endregion
}
