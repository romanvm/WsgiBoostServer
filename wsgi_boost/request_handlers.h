#pragma once
/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "request.h"
#include "response.h"
#include "utils.h"

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
		StaticRequestHandler(Request& request, Response& response) : BaseRequestHandler(request, response) {}

		// Handle request
		void handle();

	private:
		void open_file(const boost::filesystem::path& content_dir_path);
		void send_file(std::istream& content_stream, headers_type& headers);
		std::pair<std::string, std::string> parse_range(std::string& requested_range, size_t& start_pos, size_t& end_pos);
	};

	// Handles WSGI requests
	class WsgiRequestHandler : public BaseRequestHandler
	{
	private:
		std::string m_status;
		headers_type m_out_headers;
		bool m_headers_sent = false;
		boost::python::object& m_app;
		boost::python::dict m_environ;
		boost::python::object m_write;
		boost::python::object m_start_response;

		void prepare_environ();
		void send_iterable(Iterator& iterable);

	public:
		WsgiRequestHandler(Request& request, Response& response, boost::python::object& app);

		// Handle request
		void handle();
	};
}
