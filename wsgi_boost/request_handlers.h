#pragma once
/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "request.h"
#include "response.h"

#include <boost/filesystem.hpp>

#include <unordered_map>
#include <string>


namespace wsgi_boost
{
	class BaseRequestHandler
	{
	public:
		BaseRequestHandler(Request& request, Response& response) : m_request{ request }, m_response{ response } {}

		BaseRequestHandler(const BaseRequestHandler&) = delete;
		BaseRequestHandler& operator=(const BaseRequestHandler&) = delete;

		virtual ~BaseRequestHandler() {}

		virtual void handle() = 0;

	protected:
		Request& m_request;
		Response& m_response;
	};

	// Handles static content
	class StaticRequestHandler : public BaseRequestHandler
	{
	public:
		StaticRequestHandler(Request& request, Response& response, std::string& cache_control, bool use_gzip) :
			m_cache_control { cache_control }, m_use_gzip{ use_gzip },
			BaseRequestHandler(request, response) {}

		// Handle request
		void handle();

	private:
		std::string& m_cache_control;
		bool m_use_gzip;

		void open_file(const boost::filesystem::path& content_dir_path);
		void send_file(std::istream& content_stream, out_headers_t& headers);
	};

	// Handles WSGI requests
	class WsgiRequestHandler : public BaseRequestHandler
	{
	private:
		std::string m_status;
		out_headers_t m_out_headers;
		pybind11::object& m_app;
		pybind11::dict m_environ;
		pybind11::object m_write;
		pybind11::object m_start_response;
		// Reserved for future use. Now it only indicates that a response has Content-Length header.
		long long m_content_length = -1;
		std::string& m_url_scheme;
		std::string& m_host_name;
		unsigned short m_local_port;
		bool m_multithread;

		// Create write() callable: https://www.python.org/dev/peps/pep-3333/#the-write-callable
		pybind11::object create_write();
		// Create start_response() callable: https://www.python.org/dev/peps/pep-3333/#the-start-response-callable
		pybind11::object create_start_response();
		void prepare_environ();
		void send_iterable(Iterable& iterable);

	public:
		WsgiRequestHandler(Request& request, Response& response, pybind11::object& app,
			std::string& scheme, std::string& host, unsigned short local_port, bool multithread);

		// Handle request
		void handle();
	};
}
