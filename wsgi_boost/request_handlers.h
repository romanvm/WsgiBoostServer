#pragma once
/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "utils.h"

#include <boost/regex.hpp>
#include <boost/python.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#include <iostream>
#include <unordered_map>
#include <sstream>


namespace wsgi_boost
{
	class BaseRequestHandler
	{
	public:
		std::vector<std::pair<std::string, std::string>> out_headers;
		std::string status;

		BaseRequestHandler(
			std::ostream& response,
			const std::string& server_name,
			const std::string& http_version,
			const std::string& method,
			const std::string& path,
			const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers);

		virtual ~BaseRequestHandler(){}

		BaseRequestHandler(const BaseRequestHandler&) = delete;
		BaseRequestHandler(BaseRequestHandler&&) = delete;

		virtual void handle_request() = 0;

		void send_status(const std::string message = "");

		void send_http_header();	

	protected:
		std::ostream& m_response;
		const std::string& m_server_name;
		const std::string& m_http_version;
		const std::string& m_method;
		const std::string& m_path;
		const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& m_in_headers;

		void initialize_headers();

	};


	class StaticRequestHandler : public BaseRequestHandler
	{
	public:
		StaticRequestHandler(
				std::ostream& response,
				const std::string& server_name,
				const std::string& http_version,
				const std::string& method,
				const std::string& path,
				const std::string& content_dir,
				const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers,
				const boost::regex& path_regex
			) : BaseRequestHandler(response, server_name, http_version, method, path, in_headers),
			m_content_dir{ content_dir },
			m_path_regex{ path_regex }
			{}

		void handle_request();

	private:
		const std::string& m_content_dir;
		const boost::regex& m_path_regex;

		void serve_file(const boost::filesystem::path& content_dir_path);
		void send_file(std::istream& content_stream);
	};


	class WsgiRequestHandler : public BaseRequestHandler
	{
	private:
		const std::string& _host_name;
		unsigned short _port;
		const std::string& _remote_endpoint_address;
		const unsigned short& _remote_endpoint_port;
		const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& _in_headers;
		std::istream& _in_content;
		boost::python::object& _app;	

		boost::python::dict environ_;
		boost::python::object start_response;
		std::string status;

	public:
		WsgiRequestHandler(
				std::ostream& response,
				const std::string& server_name,
				const std::string& host_name,
				unsigned short port,
				const std::string& http_version,
				const std::string& method,
				const std::string& path,
				const std::string& remote_endpoint_address,
				const unsigned short& remote_endpoint_port,
				const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers,
				std::istream& in_content,
				boost::python::object& app
			) : BaseRequestHandler(response, server_name, http_version, method, path, in_headers),
			_host_name{ host_name },
			_port{ port },
			_remote_endpoint_address{ remote_endpoint_address },
			_remote_endpoint_port{ remote_endpoint_port },
			_in_headers{ in_headers },
			_in_content{ in_content },
			_app{ app }
			{
				std::function<void(boost::python::str, boost::python::list, boost::python::object)> sr{
					[this](boost::python::str stat, boost::python::list hdrs, boost::python::object exc_info = boost::python::object())
					{
						if (!exc_info.is_none())
						{
							PyErr_Print();
							exc_info = boost::python::object();
						}
						std::string status = boost::python::extract<char*>(stat);
						for (size_t i = 0; i < boost::python::len(hdrs); ++i)
						{
							boost::python::object header = hdrs[i];
							this->out_headers.emplace_back(boost::python::extract<char*>(header[0]), boost::python::extract<char*>(header[1]));
						}
						this->status = status;
					} };
				start_response = boost::python::make_function(sr,
					boost::python::default_call_policies(),
					(boost::python::arg("status"), "headers", boost::python::arg("exc_info") = boost::python::object()),
					boost::mpl::vector<void, boost::python::str, boost::python::list, boost::python::object>());
			}
		

		void handle_request()
		{
			if (_app.is_none())
			{
				send_status("500 Internal Server Error", "500: Internal server error! WSGI application is not configured.");
				return;
			}
			_prepare_environ();
			auto args = get_python_object(Py_BuildValue("(O,O)", environ_.ptr(), start_response.ptr()));
			Iterator iterable{ get_python_object(PyEval_CallObject(_app.ptr(), args.ptr())) };
			_send_iterable(iterable);			
		}

	private:
		void _prepare_environ()
		{			
			environ_["SERVER_SOFTWARE"] = m_server_name;
			environ_["REQUEST_METHOD"] = m_method;
			environ_["SCRIPT_NAME"] = "";
			environ_["PATH_INFO"] = m_path;
			environ_["QUERY_STRING"] = get_query_string(m_path);
			auto ct_iter = _in_headers.find("Content-Type");
			if (ct_iter != _in_headers.end())
			{
				environ_["CONTENT_TYPE"] = ct_iter->second;
			}
			auto cl_iter = _in_headers.find("Content-Length");
			if (cl_iter != _in_headers.end())
			{
				environ_["CONTENT_LENGTH"] = cl_iter->second;
			}
			environ_["SERVER_NAME"] = _host_name;
			environ_["SERVER_PORT"] = std::to_string(_port);
			environ_["SERVER_PROTOCOL"] = "HTTP/" + m_http_version;
			for (auto& header : _in_headers)
			{
				std::string env_header = transform_header(header.first);
				if (env_header == "HTTP_CONTENT_TYPE" || env_header == "HTTP_CONTENT_LENGTH")
				{
					continue;
				}
				if (!boost::python::extract<bool>(environ_.attr("__contains__")(env_header)))
				{
					environ_[env_header] =  header.second;
				}
				else
				{
					environ_[env_header] = environ_[env_header] + "," + header.second;
				}
			}
			environ_["wsgi.version"] = boost::python::make_tuple<int, int>(1, 0);
			environ_["wsgi.url_scheme"] = m_server_name.substr(0, 4);
			std::string in_content;
			_in_content >> in_content;
			boost::python::object input = boost::python::import("cStringIO").attr("StringIO")();
			input.attr("write")(in_content);
			environ_["wsgi.input"] = input;
			boost::python::object errors = boost::python::import("sys").attr("stderr");
			environ_["wsgi.errors"] = errors;
			environ_["wsgi.multithread"] = true;
			environ_["wsgi.multiprocess"] = false;
			environ_["wsgi.run_once"] = false;
		}

		void _send_iterable(Iterator& iterable)
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
						send_http_header(status);
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
	};
}
