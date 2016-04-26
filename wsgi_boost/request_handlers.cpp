/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "request_handlers.h"

using namespace std;
namespace py = boost::python;
namespace fs = boost::filesystem;

namespace wsgi_boost
{
#pragma region BaseRequestHandler

	BaseRequestHandler::BaseRequestHandler(
			ostream& response,
			const string& server_name,
			const string& http_version,
			const string& method,
			const string& path,
			const unordered_multimap<string, string, ihash, iequal_to>& in_headers) :
		m_response{ response }, m_server_name{ server_name }, m_http_version{ http_version },
		m_in_headers{ in_headers }, m_method{ method }, m_path{ path }
	{
		initialize_headers();
	}

	void BaseRequestHandler::send_status(const string message)
	{
		out_headers.emplace_back("Content-Length", to_string(message.length()));
		if (message != "")
		{
			out_headers.emplace_back("Content-Type", "text/plain");
		}
		send_http_header();
		m_response << message;
	}

	void BaseRequestHandler::send_http_header()
	{
		m_response << "HTTP/" << m_http_version << " " << status << "\r\n";
		for (const auto& header : out_headers)
		{
			m_response << header.first << ": " << header.second << "\r\n";
		}
		m_response << "\r\n";
	}

	void BaseRequestHandler::initialize_headers()
	{
		out_headers.emplace_back("Server", m_server_name);
		out_headers.emplace_back("Date", get_current_gmt_time());
		const auto conn_iter = m_in_headers.find("Connection");
		if (conn_iter != m_in_headers.end())
		{
			out_headers.emplace_back("Connection", conn_iter->second);
		}
	}

#pragma endregion

#pragma region StaticRequestHandler

	void StaticRequestHandler::handle_request()
	{
		const auto content_dir_path = boost::filesystem::path{ m_content_dir };
		if (!boost::filesystem::exists(content_dir_path))
		{
			status = "500 Internal Server Error";
			send_status("500: Internal server error! Invalid content directory.");
			return;
		}
		if (m_method != "GET" && m_method != "HEAD")
		{
			status = "405 Method Not Allowed";
			send_status();
			return;
		}
		serve_file(content_dir_path);
	}


	void StaticRequestHandler::serve_file(const fs::path& content_dir_path)
	{
		fs::path path = content_dir_path;
		path /= boost::regex_replace(m_path, m_path_regex, "");
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
						auto ims_iter = m_in_headers.find("If-Modified-Since");
						if (ims_iter != m_in_headers.end() && last_modified > header_to_time(ims_iter->second))
						{
							status = "304 Not Modified";
							send_status();
							return;
						}
						MimeTypes mime_types;
						string mime = mime_types[path.extension().string()];
						out_headers.emplace_back("Content-Type", mime);
						out_headers.emplace_back("Last-Modified", time_to_header(last_modified));
						auto ae_iter = m_in_headers.find("Accept-Encoding");
						if (mime_types.is_compressable(mime) && ae_iter != m_in_headers.end() && ae_iter->second.find("gzip") != std::string::npos)
						{
							boost::iostreams::filtering_istream gzstream;
							gzstream.push(boost::iostreams::gzip_compressor());
							gzstream.push(ifs);
							stringstream compressed;
							boost::iostreams::copy(gzstream, compressed);
							out_headers.emplace_back("Content-Encoding", "gzip");
							if (m_method == "HEAD")
							{
								compressed.str("");
							}
							send_file(compressed);
						}
						else
						{
							if (m_method == "GET")
							{
								send_file(ifs);
							}
							else
							{
								stringstream ss;
								ss.str("");
								send_file(ss);
							}
						}
						ifs.close();
						return;
					}
				}
			}
		}
		status = "404 Not Found";
		send_status("Error 404: Requested content not found!");
	}

	void StaticRequestHandler::send_file(istream& content_stream)
	{
		content_stream.seekg(0, ios::end);
		size_t length = content_stream.tellg();
		content_stream.seekg(0, ios::beg);
		out_headers.emplace_back("Content-Length", to_string(length));
		status = "200 OK";
		send_http_header();
		//read and send 128 KB at a time
		const size_t buffer_size = 131072;
		SafeCharBuffer buffer{ buffer_size };
		size_t read_length;
		while ((read_length = content_stream.read(buffer.data, buffer_size).gcount()) > 0)
		{
			m_response.write(buffer.data, read_length);
			m_response.flush();
		}
	}

#pragma endregion

#pragma region WsgiRequestHandler

	WsgiRequestHandler::WsgiRequestHandler(
		ostream& response,
		const string& server_name,
		const string& host_name,
		unsigned short port,
		const string& http_version,
		const string& method,
		const string& path,
		const string& remote_endpoint_address,
		const unsigned short& remote_endpoint_port,
		const unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers,
		istream& in_content,
		py::object& app
	) : BaseRequestHandler(response, server_name, http_version, method, path, in_headers),
		m_host_name{ host_name },
		m_port{ port },
		m_remote_endpoint_address{ remote_endpoint_address },
		m_remote_endpoint_port{ remote_endpoint_port },
		m_in_headers{ in_headers },
		m_in_content{ in_content },
		m_app{ app }
	{
		std::function<void(py::str, py::list, py::object)> sr{
			[this](py::str status, py::list headers, py::object exc_info = py::object())
		{
			if (!exc_info.is_none())
			{
				PyErr_Print();
				exc_info = py::object();
			}
			this->status = py::extract<char*>(status);
			for (size_t i = 0; i < py::len(headers); ++i)
			{
				py::object header = headers[i];
				this->out_headers.emplace_back(py::extract<char*>(header[0]), py::extract<char*>(header[1]));
			}
		} };
		m_start_response = py::make_function(sr, py::default_call_policies(),
			(py::arg("status"), "headers", py::arg("exc_info") = py::object()),
			boost::mpl::vector<void, py::str, py::list, py::object>());
	}


	void WsgiRequestHandler::handle_request()
	{
		if (m_app.is_none())
		{
			status = "500 Internal Server Error";
			send_status("500: Internal server error! WSGI application is not configured.");
			return;
		}
		prepare_environ();
		auto args = get_python_object(Py_BuildValue("(O,O)", m_environ.ptr(), m_start_response.ptr()));
		Iterator iterable{ get_python_object(PyEval_CallObject(m_app.ptr(), args.ptr())) };
		send_iterable(iterable);
	}


	void WsgiRequestHandler::prepare_environ()
	{
		m_environ["SERVER_SOFTWARE"] = m_server_name;
		m_environ["REQUEST_METHOD"] = m_method;
		m_environ["SCRIPT_NAME"] = "";
		m_environ["PATH_INFO"] = m_path;
		m_environ["QUERY_STRING"] = get_query_string(m_path);
		auto ct_iter = m_in_headers.find("Content-Type");
		if (ct_iter != m_in_headers.end())
		{
			m_environ["CONTENT_TYPE"] = ct_iter->second;
		}
		auto cl_iter = m_in_headers.find("Content-Length");
		if (cl_iter != m_in_headers.end())
		{
			m_environ["CONTENT_LENGTH"] = cl_iter->second;
		}
		m_environ["SERVER_NAME"] = m_host_name;
		m_environ["SERVER_PORT"] = std::to_string(m_port);
		m_environ["SERVER_PROTOCOL"] = "HTTP/" + m_http_version;
		for (auto& header : m_in_headers)
		{
			std::string env_header = transform_header(header.first);
			if (env_header == "HTTP_CONTENT_TYPE" || env_header == "HTTP_CONTENT_LENGTH")
			{
				continue;
			}
			if (!boost::python::extract<bool>(m_environ.attr("__contains__")(env_header)))
			{
				m_environ[env_header] = header.second;
			}
			else
			{
				m_environ[env_header] = m_environ[env_header] + "," + header.second;
			}
		}
		m_environ["wsgi.version"] = boost::python::make_tuple<int, int>(1, 0);
		m_environ["wsgi.url_scheme"] = m_server_name.substr(0, 4);
		std::string in_content;
		m_in_content >> in_content;
		boost::python::object input = boost::python::import("cStringIO").attr("StringIO")();
		input.attr("write")(in_content);
		m_environ["wsgi.input"] = input;
		boost::python::object errors = boost::python::import("sys").attr("stderr");
		m_environ["wsgi.errors"] = errors;
		m_environ["wsgi.multithread"] = true;
		m_environ["wsgi.multiprocess"] = false;
		m_environ["wsgi.run_once"] = false;
	}

	void WsgiRequestHandler::send_iterable(Iterator& iterable)
	{
		boost::python::object iterator = iterable.attr("__iter__")();
		bool headers_sent = false;
		while (true)
		{
			try
			{
#if PY_MAJOR_VERSION < 3
				boost::python::object chunk = iterator.attr("next")();
#else
				boost::python::object chunk = iterator.attr("__next__")();
#endif
				if (!headers_sent)
				{
					send_http_header();
					headers_sent = true;
				}
				m_response << boost::python::extract<char*>(chunk);
			}
			catch (const boost::python::error_already_set& ex)
			{
				PyObject *e, *v, *t;
				PyErr_Fetch(&e, &v, &t);
				PyErr_NormalizeException(&e, &v, &t);
				if (PyErr_GivenExceptionMatches(PyExc_StopIteration, e))
				{
					PyErr_Clear();
					break;
				}
				{
					PyErr_Restore(e, v, t);
					throw;
				}
			}
		}
	}

#pragma endregion
}
