/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "request_handlers.h"

using namespace std;
namespace py = boost::python;
namespace fs = boost::filesystem;
namespace sys = boost::system;


namespace wsgi_boost
{
#pragma region StaticRequestHandler

	void StaticRequestHandler::handle()
	{
		const auto content_dir_path = boost::filesystem::path{ m_request.content_dir };
		if (!boost::filesystem::exists(content_dir_path))
		{
			status = "500 Internal Server Error";
			m_response.send_mesage(status, "Error 500: Internal server error! Invalid content directory.");
			return;
		}
		if (m_request.method != "GET" && m_request.method != "HEAD")
		{
			status = "405 Method Not Allowed";
			m_response.send_mesage(status);
			return;
		}
		serve_file(content_dir_path);
	}


	void StaticRequestHandler::serve_file(const fs::path& content_dir_path)
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
						time_t last_modified = fs::last_write_time(path);
						string ims = m_request.get_header("If-Modified-Since");
						if (ims != "" && last_modified > header_to_time(ims))
						{
							status = "304 Not Modified";
							m_response.send_mesage(status);
							return;
						}
						MimeTypes mime_types;
						string mime = mime_types[path.extension().string()];
						headers_type out_headers;
						out_headers.emplace_back("Content-Type", mime);
						out_headers.emplace_back("Last-Modified", time_to_header(last_modified));
						if (mime_types.is_compressable(mime) && m_request.check_header("Accept-Encoding", "gzip"))
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
							send_file(ifs, out_headers);
						}
						ifs.close();
						return;
					}
				}
			}
		}
		status = "404 Not Found";
		m_response.send_mesage(status, "Error 404: Requested content not found!");
	}

	void StaticRequestHandler::send_file(istream& content_stream, headers_type& headers)
	{
		content_stream.seekg(0, ios::end);
		size_t length = content_stream.tellg();
		content_stream.seekg(0, ios::beg);
		headers.emplace_back("Content-Length", to_string(length));
		status = "200 OK";
		m_response.send_header(status, headers);
		if (m_request.method == "GET")
		{
			//read and send 128 KB at a time
			const size_t buffer_size = 131072;
			vector<char> buffer(buffer_size);
			size_t read_length;
			while ((read_length = content_stream.read(&buffer[0], buffer_size).gcount()) > 0)
			{
				sys::error_code ec = m_response.send_data(string(&buffer[0], read_length));
				if (ec)
					return;
			}
		}
	}

#pragma endregion

#pragma region WsgiRequestHandler

	WsgiRequestHandler::WsgiRequestHandler(Request& request, Response& response, py::object& app) :
		BaseRequestHandler(request, response), m_app{ app }
	{
		function<void(py::object)> wr{
			[this](py::object data)
			{
				sys::error_code ec = m_response.send_header(status, m_out_headers);
				if (ec)
					return;
				m_headers_sent = true;
				string cpp_data = py::extract<char*>(data);
				GilRelease release_gil;
				m_response.send_data(cpp_data);
			}
		};
		m_write = py::make_function(wr, py::default_call_policies(), py::args("data"), boost::mpl::vector<void, py::object>());
		function<py::object(py::str, py::list, py::object)> sr{
			[this](py::str status, py::list headers, py::object exc_info = py::object())
			{
				if (!exc_info.is_none())
				{
					py::object format_exc = py::import("traceback").attr("format_exc")();
					cerr << py::extract<char*>(format_exc) << endl;
					exc_info = py::object();
				}
				this->status = py::extract<char*>(status);
				m_out_headers.clear();
				for (size_t i = 0; i < py::len(headers); ++i)
				{
					py::object header = headers[i];
					m_out_headers.emplace_back(py::extract<char*>(header[0]), py::extract<char*>(header[1]));
				}
				return m_write;
			}
		};
		m_start_response = py::make_function(sr, py::default_call_policies(),
			(py::arg("status"), "headers", py::arg("exc_info") = py::object()),
			boost::mpl::vector<py::object, py::str, py::list, py::object>());
	}


	void WsgiRequestHandler::handle()
	{
		if (m_app.is_none())
		{
			status = "500 Internal Server Error";
			m_response.send_mesage(status, "Error 500: Internal server error! WSGI application is not set.");
			return;
		}
		prepare_environ();
		if (m_request.check_header("Expect", "100-continue"))
		{
			m_response.send_mesage("100 Continue");
		}
		py::object args = get_python_object(Py_BuildValue("(O,O)", m_environ.ptr(), m_start_response.ptr()));
		Iterator iterable{ get_python_object(PyEval_CallObject(m_app.ptr(), args.ptr())) };
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
		for (auto& header : m_request.headers)
		{
			std::string env_header = transform_header(header.first);
			if (env_header == "HTTP_CONTENT_TYPE" || env_header == "HTTP_CONTENT_LENGTH")
			{
				continue;
			}
			if (!py::extract<bool>(m_environ.attr("__contains__")(env_header)))
			{
				m_environ[env_header] = header.second;
			}
			else
			{
				m_environ[env_header] = m_environ[env_header] + "," + header.second;
			}
		}
		m_environ["wsgi.version"] = py::make_tuple<int, int>(1, 0);
		m_environ["wsgi.url_scheme"] = m_request.url_scheme;
		InputWrapper input{ m_request.connection() };
		m_environ["wsgi.input"] = input; 
		m_environ["wsgi.errors"] = py::import("sys").attr("stderr");
		m_environ["wsgi.multithread"] = true;
		m_environ["wsgi.multiprocess"] = false;
		m_environ["wsgi.run_once"] = false;
		m_environ["wsgi.file_wrapper"] = py::import("wsgiref.util").attr("FileWrapper");
	}

	void WsgiRequestHandler::send_iterable(Iterator& iterable)
	{
		py::object iterator = iterable.attr("__iter__")();
		while (true)
		{
			try
			{
#if PY_MAJOR_VERSION < 3
				std::string chunk = py::extract<char*>(iterator.attr("next")());
#else
				std::string chunk = py::extract<char*>(iterator.attr("__next__")());
#endif
				GilRelease release_gil;
				sys::error_code ec;
				if (!m_headers_sent)
				{
					ec = m_response.send_header(status, m_out_headers);
					if (ec)
						break;
					m_headers_sent = true;
				}
				ec = m_response.send_data(chunk);
				if (ec)
					break;
			}
			catch (const py::error_already_set&)
			{
				if (PyErr_ExceptionMatches(PyExc_StopIteration))
				{
					PyErr_Clear();
					break;
				}
				{
					throw;
				}
			}
		}
	}
#pragma endregion
}
