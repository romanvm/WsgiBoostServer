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

using namespace std;
namespace py = boost::python;
namespace fs = boost::filesystem;
namespace sys = boost::system;
namespace alg = boost::algorithm;


namespace wsgi_boost
{
#pragma region StaticRequestHandler

	void StaticRequestHandler::handle()
	{
		const auto content_dir_path = fs::path{ m_request.content_dir };
		if (!fs::exists(content_dir_path))
		{
			m_response.send_mesage("500 Internal Server Error",
				"Error 500: Internal server error! Invalid content directory.");
			return;
		}
		if (m_request.method != "GET" && m_request.method != "HEAD")
		{
			m_response.send_mesage("405 Method Not Allowed");
			return;
		}
		open_file(content_dir_path);
	}


	void StaticRequestHandler::open_file(const fs::path& content_dir_path)
	{
		fs::path path = content_dir_path;
		path /= boost::regex_replace(m_request.path, m_request.path_regex, "");
		if (fs::exists(path))
		{
			path = fs::canonical(path);
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
						string ims = m_request.get_header("If-Modified-Since");
						if (ims != "" && header_to_time(ims) >= last_modified)
						{
							out_headers.emplace_back("Content-Length", "0");
							m_response.send_header("304 Not Modified", out_headers, true);
							return;
						}
						string ext = path.extension().string();
						string mime = get_mime(ext);
						out_headers.emplace_back("Content-Type", mime);
						if (m_request.use_gzip && is_compressable(ext) && m_request.check_header("Accept-Encoding", "gzip"))
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
		m_response.send_mesage("404 Not Found", "Error 404: Requested content not found!");
	}


	void StaticRequestHandler::send_file(istream& content_stream, headers_type& headers)
	{
		content_stream.seekg(0, ios::end);
		size_t length = content_stream.tellg();
		headers.emplace_back("Content-Length", to_string(length));
		size_t start_pos = 0;
		size_t end_pos = length - 1;
		string requested_range = m_request.get_header("Range");
		pair<string, string> range;
		if (requested_range != "" && ((range = parse_range(requested_range)) != pair<string, string>()))
		{
			if (range.first != string())
			{
				start_pos = stoull(range.first);
			}
			else
			{
				range.first = to_string(start_pos);
			}
			if (range.second != string())
			{
				end_pos = stoull(range.second);
			}
			else
			{
				range.second = to_string(end_pos);
			}
			if (start_pos > end_pos || start_pos >= length || end_pos >= length)
			{
				m_response.send_mesage("416 Range Not Satisfiable");
				return;
			}
			else
			{
				headers.emplace_back("Content-Range", "bytes " + range.first + "-" + range.second + "/" + to_string(length));
				m_response.send_header("206 Partial Content", headers, true);
			}
		}
		else
		{
			m_response.send_header("200 OK", headers, true);
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
				sys::error_code ec = m_response.send_data(string(&buffer[0], read_length), true);
				if (ec)
					return;
				bytes_left -= read_length;
			}
		}
	}

#pragma endregion

#pragma region WsgiRequestHandler

	WsgiRequestHandler::WsgiRequestHandler(Request& request, Response& response, py::object& app, bool async) :
		BaseRequestHandler(request, response), m_app{ app }, m_async{ async }
	{
		// Create write() callable: https://www.python.org/dev/peps/pep-3333/#the-write-callable
		function<void(py::object)> wr{
			[this](py::object data)
			{
				sys::error_code ec = m_response.send_header(m_status, m_out_headers);
				if (ec)
					return;
				m_headers_sent = true;
				string cpp_data = py::extract<string>(data);
				GilRelease release_gil;
				m_response.send_data(cpp_data);
			}
		};
		m_write = py::make_function(wr, py::default_call_policies(), py::args("data"), boost::mpl::vector<void, py::object>());
		// Create start_response() callable: https://www.python.org/dev/peps/pep-3333/#the-start-response-callable
		function<py::object(py::str, py::list, py::object)> sr{
			[this](py::str status, py::list headers, py::object exc_info = py::object())
			{
				if (!exc_info.is_none() && this->m_headers_sent)
				{
					py::object type = exc_info[0];
					py::object value = exc_info[1];
					py::object traceback = exc_info[2];
					PyErr_Restore(type.ptr(), value.ptr(), traceback.ptr());
					throw FatalWsgiAppError();
				}
				this->m_status = py::extract<string>(status);
				m_out_headers.clear();
				bool has_cont_len = false;
				bool has_chunked = false;
				for (size_t i = 0; i < py::len(headers); ++i)
				{
					py::object header = headers[i];
					string header_name = py::extract<string>(header[0]);
					string header_value = py::extract<string>(header[1]);
					// If a WSGI app does not provide Content-Length header (e.g. Django)
					// we use Transfer-Encoding: chunked
					if (alg::iequals(header_name, "Content-Length"))
						has_cont_len = true;
					if (alg::iequals(header_name, "Transfer-Encoding") && alg::icontains(header_value, "chunked"))	
						has_chunked = true;
					m_out_headers.emplace_back(header_name, header_value);
				}
				if (!(has_cont_len || has_chunked))
				{
					this->m_send_chunked = true;
					m_out_headers.emplace_back("Transfer-Encoding", "chunked");
				}
				return m_write;
			}
		};
		m_start_response = py::make_function(sr, py::default_call_policies(),
			(py::arg("status"), py::arg("headers"), py::arg("exc_info") = py::object()),
			boost::mpl::vector<py::object, py::str, py::list, py::object>());
	}


	void WsgiRequestHandler::handle()
	{
		if (m_app.is_none())
		{
			m_response.send_mesage("500 Internal Server Error",
				"Error 500: Internal server error! WSGI application is not set.");
			return;
		}
		prepare_environ();
		if (m_request.check_header("Expect", "100-continue"))
		{
			m_response.send_mesage("100 Continue");
		}
		Iterable iterable{ m_app(m_environ, m_start_response) };
		send_iterable(iterable);
	}


	void WsgiRequestHandler::prepare_environ()
	{
		m_environ["REQUEST_METHOD"] = m_request.method;
		m_environ["SCRIPT_NAME"] = "";
		pair<string, string> path_and_query = split_path(m_request.path);
		m_environ["PATH_INFO"] = path_and_query.first;
		m_environ["QUERY_STRING"] = path_and_query.second;
		string ct = m_request.get_header("Content-Type");
		if (ct != "")
		{
			m_environ["CONTENT_TYPE"] = ct;
		}
		string cl = m_request.get_header("Content-Length");
		if (cl != "")
		{
			m_environ["CONTENT_LENGTH"] = cl;
		}
		m_environ["SERVER_NAME"] = m_request.host_name;
		m_environ["SERVER_PORT"] = to_string(m_request.local_endpoint_port);
		m_environ["SERVER_PROTOCOL"] = m_request.http_version;
		for (const auto& header : m_request.headers)
		{
			if (alg::iequals(header.first, "Content-Length") || alg::iequals(header.first, "Content-Type"))
				continue;
			string env_header = header.first;
			transform_header(env_header);
			if (!m_environ.has_key(env_header))
				m_environ[env_header] = header.second;
			else
				m_environ[env_header] = m_environ[env_header] + "," + header.second;
		}
		m_environ["REMOTE_ADDR"] = m_environ["REMOTE_HOST"] = m_request.remote_address();
		m_environ["REMOTE_PORT"] = to_string(m_request.remote_port());
		m_environ["wsgi.version"] = py::make_tuple<int, int>(1, 0);
		m_environ["wsgi.url_scheme"] = m_request.url_scheme;
		InputWrapper input{ m_request.connection(), m_async };
		m_environ["wsgi.input"] = input; 
		m_environ["wsgi.errors"] = py::import("sys").attr("stderr");
		m_environ["wsgi.multithread"] = true;
		m_environ["wsgi.multiprocess"] = false;
		m_environ["wsgi.run_once"] = false;
		m_environ["wsgi.file_wrapper"] = py::import("wsgiref.util").attr("FileWrapper");
	}

	void WsgiRequestHandler::send_iterable(Iterable& iterable)
	{
		py::object iterator = iterable.attr("__iter__")();
		while (true)
		{
			try
			{
#if PY_MAJOR_VERSION < 3
				std::string chunk = py::extract<string>(iterator.attr("next")());
#else
				std::string chunk = py::extract<string>(iterator.attr("__next__")());
#endif
				GilRelease release_gil;
				sys::error_code ec;
				if (!m_headers_sent)
				{
					ec = m_response.send_header(m_status, m_out_headers);
					if (ec)
						break;
					m_headers_sent = true;
				}
				if (m_send_chunked)
				{
					size_t length = chunk.length();
					// Skip 0-length chunks, if any
					if (length == 0)
						continue;
					chunk = hex(length) + "\r\n" + chunk + "\r\n";
				}
				ec = m_response.send_data(chunk, m_async);
				if (ec)
					break;
			}
			catch (const py::error_already_set&)
			{
				if (PyErr_ExceptionMatches(PyExc_StopIteration))
				{
					PyErr_Clear();
					if (m_send_chunked)
						m_response.send_data("0\r\n\r\n");
					break;
				}
				if (m_headers_sent)
					throw FatalWsgiAppError();
				throw;
			}
		}
	}
#pragma endregion
}
