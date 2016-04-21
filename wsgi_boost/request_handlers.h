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


namespace WsgiBoost
{
	class BaseRequestHandler
	{
	protected:
		std::ostream& _response;
		const std::string& _server_name;
		const std::string& _http_version;
		const std::string& _method;
		const std::string& _path;
		const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& _in_headers;
		std::vector<std::pair<std::string, std::string>> _out_headers;

	public:
		BaseRequestHandler(
			std::ostream& response,
			const std::string& server_name,
			const std::string& http_version,
			const std::string& method,
			const std::string& path,
			const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers
		) :
			_response{ response },
			_server_name{ server_name },
			_http_version{ http_version },
			_in_headers{ in_headers },
			_method{ method },
			_path{ path }
		{
			_initialize_headers();
		}

		virtual ~BaseRequestHandler(){}

		BaseRequestHandler(const BaseRequestHandler&) = delete;
		BaseRequestHandler(BaseRequestHandler&&) = delete;

		virtual void handle_request() = 0;

		void send_code(const std::string& code, const std::string message = "")
		{
			_out_headers.emplace_back("Content-Length", std::to_string(message.length()));
			if (message != "")
			{
				_out_headers.emplace_back("Content-Type", "text/plain");
			}
			_send_http_header(code);
			_response << message;
		}

	protected:
		void _initialize_headers()
		{
			_out_headers.emplace_back("Server", _server_name);
			_out_headers.emplace_back("Date", get_current_gmt_time());
			const auto conn_iter = _in_headers.find("Connection");
			if (conn_iter != _in_headers.end())
			{
				_out_headers.emplace_back("Connection", conn_iter->second);
			}
		}

		void _send_http_header(const std::string& code)
		{
			_response << "HTTP/" << _http_version << " " << code << "\r\n";
			for (const auto& header : _out_headers)
			{
				_response << header.first << ": " << header.second << "\r\n";
			}
			_response << "\r\n";
		}
	};


	class StaticRequestHandler : public BaseRequestHandler
	{
	private:
		const std::string& _content_dir;
		const boost::regex& _path_regex;

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
			_content_dir{ content_dir },
			_path_regex{ path_regex }
			{}

		void handle_request()
		{
			const auto content_dir_path = boost::filesystem::path{ _content_dir };
			if (!boost::filesystem::exists(content_dir_path))
			{
				send_code("500 Internal Server Error", "500: Internal server error! Invalid content directory.");
				return;
			}
			if (_method != "GET" && _method != "HEAD")
			{
				std::string code = "405 Method Not Allowed";
				send_code(code, code);
				return;
			}
			_serve_file(content_dir_path);
		}

	private:
		void _serve_file(const boost::filesystem::path& content_dir_path)
		{
			boost::filesystem::path path = content_dir_path;
			path /= boost::regex_replace(_path, _path_regex, "");
			if (boost::filesystem::exists(path))
			{
				path = boost::filesystem::canonical(path);
				// Checking if path is inside content_dir
				if (std::distance(content_dir_path.begin(), content_dir_path.end()) <= std::distance(path.begin(), path.end()) &&
					std::equal(content_dir_path.begin(), content_dir_path.end(), path.begin()))
				{
					if (boost::filesystem::is_directory(path))
					{
						path /= "index.html";
					}
					if (boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path))
					{
						std::ifstream ifs;
						ifs.open(path.string(), std::ifstream::in | std::ios::binary);
						if (ifs)
						{
							time_t last_modified = boost::filesystem::last_write_time(path);
							auto ims_iter = _in_headers.find("If-Modified-Since");
							if (ims_iter != _in_headers.end() && last_modified > header_to_time(ims_iter->second))
							{
								send_code("304 Not Modified");
								return;
							}
							MimeTypes mime_types;
							std::string mime = mime_types[path.extension().string()];			
							_out_headers.emplace_back("Content-Type", mime);
							_out_headers.emplace_back("Last-Modified", time_to_header(last_modified));
							auto ae_iter = _in_headers.find("Accept-Encoding");
							if (mime_types.is_compressable(mime) && ae_iter != _in_headers.end() && ae_iter->second.find("gzip") != std::string::npos)
							{
								boost::iostreams::filtering_istream gzstream;
								gzstream.push(boost::iostreams::gzip_compressor());
								gzstream.push(ifs);
								std::stringstream compressed;
								boost::iostreams::copy(gzstream, compressed);
								_out_headers.emplace_back("Content-Encoding", "gzip");
								if (_method == "HEAD")
								{
									compressed.str("");
								}
								_send_file(compressed);
							}
							else
							{
								if (_method == "GET")
								{
									_send_file(ifs);
								}
								else
								{
									std::stringstream ss;
									ss.str("");
									_send_file(ss);
								}
							}
							ifs.close();
							return;
						}
					}
				}
			}
			std::cerr << "Invalid path: " << path.string() << std::endl;
			send_code("404 Not Found", "Error 404: Requested content not found!");
		}

		void _send_file(std::istream& content_stream)
		{
			content_stream.seekg(0, std::ios::end);
			size_t length = content_stream.tellg();
			content_stream.seekg(0, std::ios::beg);
			_out_headers.emplace_back("Content-Length", std::to_string(length));
			_send_http_header("200 OK");
			//read and send 128 KB at a time
			const size_t buffer_size = 131072;
			SafeCharBuffer buffer{ buffer_size };
			size_t read_length;
			while ((read_length = content_stream.read(buffer.data, buffer_size).gcount()) > 0)
			{
				_response.write(buffer.data, read_length);
				_response.flush();
			}
		}
	};

	
	class WsgiRequestHandler : public BaseRequestHandler
	{
	private:
		const std::string& _remote_endpoint_address;
		const unsigned short& _remote_endpoint_port;
		const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& _in_headers;
		std::istream& _in_content;
		boost::python::object& _app;	

		boost::python::dict environ_;

	public:
		WsgiRequestHandler(
				std::ostream& response,
				const std::string& server_name,
				const std::string& http_version,
				const std::string& method,
				const std::string& path,
				const std::string& remote_endpoint_address,
				const unsigned short& remote_endpoint_port,
				const std::unordered_multimap<std::string, std::string, ihash, iequal_to>& in_headers,
				std::istream& in_content,
				boost::python::object& app
			) : BaseRequestHandler(response, server_name, http_version, method, path, in_headers),
			_remote_endpoint_address{ remote_endpoint_address },
			_remote_endpoint_port{ remote_endpoint_port },
			_in_headers{ in_headers },
			_in_content{ in_content },
			_app{ app }
			{}

		void handle_request()
		{
			if (_app.is_none())
			{
				send_code("500 Internal Server Error", "500: Internal server error! Application is not configured.");
				return;
			}
			_prepare_environ();
		}

	private:
		void _prepare_environ()
		{
			environ_["REQUEST_METHOD"] = _method;
			environ_["SCRIPT_NAME"] = "";
			environ_["PATH_INFO"] = _path;
			environ_["QUERY_STRING"] = get_query_string(_path);
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
		}
	};
}
