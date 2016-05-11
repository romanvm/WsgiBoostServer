/*
Request and related classes

Based on: https://github.com/eidheim/Simple-Web-Server

Copyright (c) 2014 Ole Christian Eidheim
Copyright (c) 2016 Roman Miroshnychenko (modified version)

License: MIT, see License.txt
*/
#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/functional/hash.hpp>
#include <boost/regex.hpp>

#include <iostream>
#include <unordered_map>


namespace wsgi_boost
{
	// Moved from server_http.h
	// Based on http://www.boost.org/doc/libs/1_60_0/doc/html/unordered/hash_equality.html
	class iequal_to
	{
	public:
		bool operator()(const std::string &key1, const std::string &key2) const
		{
			return boost::algorithm::iequals(key1, key2);
		}
	};


	class ihash
	{
	public:
		size_t operator()(const std::string &key) const
		{
			std::size_t seed = 0;
			for (auto &c : key)
				boost::hash_combine(seed, std::tolower(c));
			return seed;
		}
	};


	class Content : public std::istream
	{
	public:
		explicit Content(boost::asio::streambuf &streambuf) : std::istream(&streambuf), streambuf(streambuf) {}

		size_t size()
		{
			return streambuf.size();
		}

		std::string string()
		{
			std::stringstream ss;
			ss << rdbuf();
			return ss.str();
		}

	private:
		boost::asio::streambuf &streambuf;
	};


	struct Request
	{
		explicit Request(boost::asio::io_service &io_service, std::shared_ptr<boost::asio::ip::tcp::socket> socket) :
			content(streambuf), strand(io_service), socket(socket), timer(io_service) {}

		std::string method;
		std::string path;
		std::string http_version;
		std::string content_dir;
		std::string remote_endpoint_address;
		std::string server_name;
		std::string host_name;
		std::string url_scheme;

		Content content;

		std::unordered_multimap<std::string, std::string, ihash, iequal_to> header;

		boost::regex path_regex;

		unsigned short remote_endpoint_port;
		unsigned short local_endpoint_port;

		boost::asio::streambuf streambuf;

		boost::asio::strand strand;

		boost::asio::deadline_timer timer;
		size_t timeout;

		std::shared_ptr<boost::asio::ip::tcp::socket> socket;

		unsigned long long content_length;
		unsigned long long bytes_left = 0;

		void read_remote_endpoint_data()
		{
			try
			{
				remote_endpoint_address = socket->lowest_layer().remote_endpoint().address().to_string();
				remote_endpoint_port = socket->lowest_layer().remote_endpoint().port();
			}
			catch (const std::exception& e) {}
		}

		// Check if a certain header is present and includes a requested string
		bool check_header(const std::string& hdr, const std::string& val)
		{
			auto iter = header.find(hdr);
			return iter != header.end() && iter->second.find(val) != std::string::npos;
		}

		std::string get_header(const std::string& hdr)
		{
			auto iter = header.find(hdr);
			if (iter != header.end())
			{
				return iter->second;
			}
			return "";
		}

		std::string read(unsigned long long size = 0)
		{
			std::ostringstream ss;
			if (bytes_left > 0)
			{			
				boost::asio::spawn(strand, [this, &size, &ss](boost::asio::yield_context yc)
				{
					std::istream stream{ &streambuf };
					boost::system::error_code ec;
					size_t bytes_transferred = boost::asio::async_read(*socket, streambuf, boost::asio::transfer_exactly(std::min(bytes_left, size)), yc[ec]);
					if (!ec)
					{
						bytes_left -= bytes_transferred;
						ss << stream.rdbuf();
					}
				});
			}
			return ss.str();
		}

		std::string readline()
		{
			std::ostringstream ss;
			if (bytes_left > 0)
			{
				boost::asio::spawn(strand, [this, &ss](boost::asio::yield_context yc)
				{
					std::istream stream{ &streambuf };
					boost::system::error_code ec;
					size_t bytes_transferred = boost::asio::async_read_until(*socket, streambuf, "\n", yc[ec]);
					if (!ec)
					{
						bytes_left -= bytes_transferred;
						ss << stream.rdbuf();
					}
				});
			}
			return ss.str();
		}

		void set_timeout()
		{
			timer.expires_from_now(boost::posix_time::seconds(timeout));
			timer.async_wait([this](const boost::system::error_code& ec)
				{
					if (!ec)
					{
						boost::system::error_code ec;
						socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
						socket->lowest_layer().close();
					}
				});
		}
	};
}
