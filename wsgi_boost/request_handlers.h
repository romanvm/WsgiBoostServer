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
		const std::string& m_host_name;
		unsigned short m_port;
		const std::string& m_remote_endpoint_address;
		const unsigned short& m_remote_endpoint_port;
		const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& m_in_headers;
		std::istream& m_in_content;
		boost::python::object& m_app;
		boost::python::dict m_environ;
		boost::python::object m_start_response;

		void prepare_environ();
		void send_iterable(Iterator& iterable);

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
			boost::python::object& app);

		void handle_request();		
	};
}
