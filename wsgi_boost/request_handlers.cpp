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


}
