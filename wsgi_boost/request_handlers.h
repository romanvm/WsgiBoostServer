#pragma once
/*
Request handlers for WSGI apps and static files

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "utils.h"
#include "request.h"

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

		BaseRequestHandler(std::ostream& response, std::shared_ptr<Request> request);

		virtual ~BaseRequestHandler(){}

		BaseRequestHandler(const BaseRequestHandler&) = delete;
		BaseRequestHandler(BaseRequestHandler&&) = delete;

		virtual void handle_request() = 0;

		void send_status(const std::string message = "");

		void send_http_header();	

	protected:
		std::ostream& m_response;
		std::shared_ptr<Request> m_request;

		void initialize_headers();

	};


	class StaticRequestHandler : public BaseRequestHandler
	{
	public:
		StaticRequestHandler(std::ostream& response, std::shared_ptr<Request> request) : BaseRequestHandler(response, request) {}

		void handle_request();

	private:
		void serve_file(const boost::filesystem::path& content_dir_path);
		void send_file(std::istream& content_stream);
	};


	class WsgiRequestHandler : public BaseRequestHandler
	{
	private:
		boost::python::object& m_app;
		boost::python::dict m_environ;
		boost::python::object m_start_response;

		void prepare_environ();
		void send_iterable(Iterator& iterable);

	public:
		WsgiRequestHandler(std::ostream& response, std::shared_ptr<Request> request, boost::python::object& app);

		void handle_request();		
	};
}
