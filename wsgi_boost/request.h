#pragma once
/*
HTTP request

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "connection.h"

#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>

#include <unordered_map>
#include <cctype>
#include <string>

namespace wsgi_boost
{
	// 2 structs below are used for unordered_map with case-insensitive string keys

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


	// HTTP request parameters
	class Request
	{
	public:
		std::string method;
		std::string path;
		std::string http_version;
		std::unordered_map<std::string, std::string, ihash, iequal_to> headers;
		boost::regex path_regex;
		std::string content_dir;
		std::string url_scheme;
		std::string host_name;
		unsigned short local_endpoint_port;
		bool use_gzip;

		Request(const Request&) = delete;
		Request& operator=(const Request&) = delete;

		explicit Request(Connection& connection) : m_connection{ connection } {}

		// Parse HTTP request headers
		boost::system::error_code parse_header();

		// Check if a header contains a specific value
		bool check_header(const std::string& header, std::string value);

		// Get header value or "" if the header is missing
		std::string get_header(const std::string& header);

		// Check if the connection is persistent (keep-alive)
		bool persistent();

		// Get Connection object for this request
		Connection& connection() const;

		// Get remote endpoint address
		std::string remote_address();

		// Get remote endpoint port
		unsigned short remote_port();

	private:
		Connection& m_connection;
	};
}
