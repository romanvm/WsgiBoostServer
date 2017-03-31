/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "request_handlers.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

using namespace std;
namespace py = pybind11;
namespace fs = boost::filesystem;
namespace sys = boost::system;
namespace alg = boost::algorithm;


namespace wsgi_boost
{
#pragma region StaticRequestHandler

	void StaticRequestHandler::handle()
	{
		if (m_request.method != "GET" && m_request.method != "HEAD")
		{
			m_response.send_mesage("405 Method Not Allowed", "Invalid HTTP method! Only GET and HEAD are allowed.");
			return;
		}
		const auto content_dir_path = fs::path{ m_request.content_dir };
		if (!fs::exists(content_dir_path))
		{
			m_response.send_html("500 Internal Server Error",
				"Error 500",
				"Internal Server Error",
				"Invalid static content directory is configured.");
			return;
		}
		open_file(content_dir_path);
	}


	void StaticRequestHandler::open_file(const fs::path& content_dir_path)
	{
		fs::path path = content_dir_path;
		path /= boost::regex_replace(m_request.path, m_request.path_regex, "");
		sys::error_code ec;
		path = fs::canonical(path, ec);
		if (!ec)
		{
			// Checking if path is inside content_dir
			if (distance(content_dir_path.begin(), content_dir_path.end()) <= distance(path.begin(), path.end()) &&
				equal(content_dir_path.begin(), content_dir_path.end(), path.begin()))
			{
				if (fs::is_directory(path))
				{
					path /= "index.html";
				}
				if (fs::exists(path) && fs::is_regular_file(path))
				{
					ifstream ifs;
					ifs.open(path.string(), ifstream::in | ios::binary);
					if (ifs)
					{
						headers_type out_headers;
						out_headers.emplace_back("Cache-Control", m_cache_control);
						time_t last_modified = fs::last_write_time(path);
						out_headers.emplace_back("Last-Modified", time_to_header(last_modified));
						// Use hex representation of the last modified POSIX timestamp as ETag
						string etag = "\"" + hex(static_cast<size_t>(last_modified)) + "\"";
						out_headers.emplace_back("ETag", etag);
						string ims = m_request.get_header("If-Modified-Since");
						if (m_request.get_header("If-None-Match") == etag || (!ims.empty() && header_to_time(ims) >= last_modified))
						{
							out_headers.emplace_back("Content-Length", "0");
							m_response.send_header("304 Not Modified", out_headers);
							ifs.close();
							return;
						}
						string ext = path.extension().string();
						out_headers.emplace_back("Content-Type", get_mime(ext));
						if (m_use_gzip && m_request.check_header("Accept-Encoding", "gzip") && is_compressable(ext))
						{
							boost::iostreams::filtering_istream gzstream;
							gzstream.push(boost::iostreams::gzip_compressor());
							gzstream.push(ifs);
							stringstream compressed;
							boost::iostreams::copy(gzstream, compressed);
							out_headers.emplace_back("Content-Encoding", "gzip");
							send_file(compressed, out_headers);
						}
						else
						{
							out_headers.emplace_back("Accept-Ranges", "bytes");
							send_file(ifs, out_headers);
						}
						ifs.close();
						return;
					}
				}
			}
		}
		m_response.send_html("404 Not Found",
			"Error 404", "Not Found",
			"The requested path <code>" + m_request.path + "</code> was not found on this server.");
	}


	void StaticRequestHandler::send_file(istream& content_stream, headers_type& headers)
	{
		content_stream.seekg(0, ios::end);
		size_t length = content_stream.tellg();
		size_t start_pos = 0;
		size_t end_pos = length - 1;
		string requested_range = m_request.get_header("Range");
		pair<string, string> range;
		if (!requested_range.empty() && ((range = parse_range(requested_range)) != pair<string, string>()))
		{
			if (!range.first.empty())
				start_pos = stoull(range.first);
			else
				range.first = to_string(start_pos);
			if (!range.second.empty())
				end_pos = stoull(range.second);
			else
				range.second = to_string(end_pos);
			if (start_pos > end_pos || start_pos >= length || end_pos >= length)
			{
				m_response.send_mesage("416 Range Not Satisfiable", "Invalid bytes range!");
				return;
			}
			else
			{
				headers.emplace_back("Content-Length", to_string(end_pos - start_pos));
				headers.emplace_back("Content-Range", "bytes " + range.first + "-" + range.second + "/" + to_string(length));
				m_response.send_header("206 Partial Content", headers);
			}
		}
		else
		{
			headers.emplace_back("Content-Length", to_string(length));
			m_response.send_header("200 OK", headers);
		}
		if (m_request.method == "GET")
		{
			if (start_pos > 0)
				content_stream.seekg(start_pos);
			else
				content_stream.seekg(0, ios::beg);
			const size_t buffer_size = 131072;
			vector<char> buffer(buffer_size);
			size_t read_length;
			size_t bytes_left = end_pos - start_pos + 1;
			while (bytes_left > 0 &&
				((read_length = content_stream.read(&buffer[0], min(bytes_left, buffer_size)).gcount()) > 0))
			{
				sys::error_code ec = m_response.send_data(string{ &buffer[0], read_length });
				if (ec)
					return;
				bytes_left -= read_length;
			}
		}
	}

#pragma endregion

#pragma region WsgiRequestHandler

	WsgiRequestHandler::WsgiRequestHandler(Request& request, Response& response, py::object& app,
		string& scheme, string& host, unsigned short local_port, bool multithread) :
		BaseRequestHandler(request, response), m_app{ app },
		m_url_scheme{ scheme }, m_host_name{ host },
		m_local_port{ local_port }, m_multithread{ multithread }
	{
		m_write = create_write();
		m_start_response = create_start_response();
	}


	py::object WsgiRequestHandler::create_write()
	{
		auto wr = [this](py::bytes data)
		{
			string cpp_data{ data };
			py::gil_scoped_release release_gil;
			// Read/write operations executed from inside Python must be syncronous!
			sys::error_code ec = this->m_response.send_header(this->m_status, this->m_out_headers, false);
			if (ec)
				return;
			size_t data_len = cpp_data.length();
			if (data_len > 0)
			{
				if (this->m_content_length == -1)
					cpp_data = hex(data_len) + "\r\n" + cpp_data + "\r\n";
				// Read/write operations executed from inside Python must be syncronous!
				this->m_response.send_data(cpp_data, false);
			}
		};
		return py::cpp_function(wr, py::arg("data"));
	}

	py::object WsgiRequestHandler::create_start_response()
	{
		auto sr = [this](py::str status, py::list headers, py::object exc_info = py::none())
		{
			if (!exc_info.is_none() && this->m_response.header_sent())
			{
				py::tuple exc = exc_info;
				py::object t = exc[0];
				py::object v = exc[1];
				py::object tb = exc[2];
				cerr << "The WSGI app passed exc_info after sending headers to the client!\n";
				PyErr_Restore(t.ptr(), v.ptr(), tb.ptr());
				throw py::error_already_set();
			}
			this->m_status.assign(status);
			this->m_out_headers.clear();
			exc_info = py::none();
			for (const auto& h : headers)
			{
				py::tuple header = py::reinterpret_borrow<py::tuple>(h);
				string header_name = header[0].cast<string>();
				string header_value = header[1].cast<string>();
				if (alg::iequals(header_name, "Content-Length"))
				{
					try
					{
						this->m_content_length = stoll(header_value);
					}
					catch (const logic_error&) {}
				}
				this->m_out_headers.emplace_back(header_name, header_value);
			}
			if (this->m_content_length == -1)
			{
				// If a WSGI app does not provide Content-Length header (e.g. Django)
				// we use Transfer-Encoding: chunked
				this->m_out_headers.emplace_back("Transfer-Encoding", "chunked");
			}
			return this->m_write;
		};
		return py::cpp_function(sr, py::arg("status"), py::arg("headers"), py::arg("exc_info") = py::none());
	}

	void WsgiRequestHandler::prepare_environ()
	{
		m_environ["REQUEST_METHOD"] = m_request.method;
		m_environ["SCRIPT_NAME"] = "";
		pair<string, string> path_and_query = split_path(m_request.path);
		m_environ["PATH_INFO"] = path_and_query.first;
		m_environ["QUERY_STRING"] = path_and_query.second;
		m_environ["CONTENT_TYPE"] = m_request.get_header("Content-Type");
		m_environ["CONTENT_LENGTH"] = m_request.get_header("Content-Length");
		m_environ["SERVER_NAME"] = m_host_name;
		m_environ["SERVER_PORT"] = to_string(m_local_port);
		m_environ["SERVER_PROTOCOL"] = m_request.http_version;
		m_environ["REMOTE_ADDR"] = m_request.remote_address();
		m_environ["REMOTE_HOST"] = m_request.remote_address();
		m_environ["REMOTE_PORT"] = to_string(m_request.remote_port());
		for (const auto& header : m_request.headers)
		{
			string env_header = header.first;
			transform_header(env_header);
			if (env_header == "HTTP_CONTENT_TYPE" || env_header == "HTTP_CONTENT_LENGTH")
				continue;
			// Headers are already checked for duplicates during parsing
			m_environ[env_header.c_str()] = header.second;
		}
		m_environ["wsgi.version"] = py::make_tuple(1, 0);
		m_environ["wsgi.url_scheme"] = m_url_scheme;
		m_environ["wsgi.input"] = InputStream{ m_request.connection() };
		m_environ["wsgi.errors"] = ErrorStream{};
		m_environ["wsgi.file_wrapper"] = FileWrapper{};
		m_environ["wsgi.multithread"] = m_multithread;
		m_environ["wsgi.multiprocess"] = false;
		m_environ["wsgi.run_once"] = false;
	}

	void WsgiRequestHandler::handle()
	{
		if (m_app.is_none())
		{
			m_response.send_html("500 Internal Server Error",
				"Error 500", "Internal Server Error",
				"A WSGI application is not set.");
			return;
		}
		prepare_environ();
		Iterable iterable{ m_app(m_environ, m_start_response) };
		send_iterable(iterable);
	}


	void WsgiRequestHandler::send_iterable(Iterable& iterable)
	{
		py::object iterator = iterable.attr("__iter__")();
		while (true)
		{
			try
			{
				py::bytes py_chunk = iterator.attr("__next__")();
				string chunk{ py_chunk };
				// Releasing GIL around async operations not only allows other Python threads to run
				// but also allows io_service to safely transfer control to another coroutine
				// that may acquire GIL again.
				// I found this scheme by accident and if we do not release GIL at this point
				// Python will crash!
				py::gil_scoped_release release_gil;
				sys::error_code ec;
				if (!m_response.header_sent())
				{
					ec = m_response.send_header(m_status, m_out_headers);
					if (ec)
						break;
				}
				if (m_content_length == -1)
				{
					size_t length = chunk.length();
					// Skip 0-length chunks, if any
					if (length == 0)
						continue;
					chunk = hex(length) + "\r\n" + chunk + "\r\n";
				}
				ec = m_response.send_data(chunk);
				if (ec)
					break;
			}
			catch (py::error_already_set& ex)
			{
				ex.restore();
				if (PyErr_ExceptionMatches(PyExc_StopIteration))
				{
					PyErr_Clear();
					if (m_content_length == -1)
						m_response.send_data("0\r\n\r\n");
					break;
				}
				throw py::error_already_set();
			}
		}
	}
#pragma endregion
}
