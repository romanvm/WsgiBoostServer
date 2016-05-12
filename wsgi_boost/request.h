#pragma once
/*
HTTP request

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "connection.h"

#include<boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <unordered_map>
#include <cctype>

namespace wsgi_boost
{
	struct iequal_to
	{
		bool operator()(const std::string &key1, const std::string &key2) const
		{
			return boost::algorithm::iequals(key1, key2);
		}
	};


	struct ihash
	{
		size_t operator()(const std::string &key) const
		{
			std::size_t seed = 0;
			for (auto &c : key)
				boost::hash_combine(seed, std::tolower(c));
			return seed;
		}
	};


	class Request
	{
	public:
		std::string method;
		std::string path;
		std::string http_version;
		std::unordered_map<std::string, std::string, ihash, iequal_to> headers;

		Request(const Request&) = delete;
		Request& operator=(const Request&) = delete;

		explicit Request(Connection& connection) : m_connection{ connection }
		{
			read_remote_endpoint_data();
		}

		bool parse_header();

		// Check if a header contains a specific value
		bool check_header(const std::string& header, std::string value);

		// Get header value or "" if the header is missing
		std::string get_header(const std::string& header);

		// Read remote IP-address and port from socket
		void read_remote_endpoint_data();

		std::string post_content();

		std::string remote_address()
		{
			return m_remote_address;
		}

		unsigned short remote_port()
		{
			return m_remote_port;
		}

		bool persistent()
		{
			return check_header("Connection", "keep-alive") || http_version == "HTTP/1.1";
		}

		Connection& connection()
		{
			return m_connection;
		}

	private:
		Connection& m_connection;
		std::string m_remote_address;
		unsigned short m_remote_port;
	};
}
