#pragma once
/*
Various utility functions

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "constants.h"

#include <boost/algorithm/string.hpp>
#include <boost/python.hpp>
#include <boost/regex.hpp>

#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <cctype>
#include <mutex>
#include <queue>
#include <iostream>
#include <thread>


namespace wsgi_boost
{
#pragma region functions

	// Converts POSIX time to HTTP header format
	inline std::string time_to_header(time_t posix_time)
	{
		char buffer[40];
		std::locale::global(std::locale("C"));
		std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&posix_time));
		return std::string{ buffer };
	}


	// Parses HTTP "time" headers to POSIX time
	inline time_t header_to_time(const std::string& time_string)
	{
		tm t;
#ifdef _WIN32
		std::stringstream ss{ time_string };
		ss.imbue(std::locale("C"));
		ss >> std::get_time(&t, "%a, %d %b %Y %H:%M:%S GMT");
		return _mkgmtime(&t);
#else
		std::locale::global(std::locale("C"));
		strptime(time_string.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &t);
		return timegm(&t);
#endif
	}


	// Get current GMT time in HTTP header format
	inline std::string get_current_gmt_time()
	{
		return time_to_header(std::time(nullptr));
	}


	// Wraps a raw PyObject* into a boost::python::object
	inline boost::python::object get_python_object(PyObject* pyobj)
	{
		return boost::python::object(boost::python::handle<>(pyobj));
	}


	// Splits a full path into a path proper and a query string
	inline std::pair<std::string, std::string> split_path(const std::string& path)
	{
		size_t pos = path.find("?");
		if (pos != std::string::npos)
		{
			return std::pair<std::string, std::string>{path.substr(0, pos), path.substr(pos + 1)};
		}
		return std::pair<std::string, std::string>(path, "");
	}


	// Transform a HTTP header to WSGI environ format HTTP_
	inline void transform_header(std::string& header)
	{
		for (auto& ch : header)
		{
			if (ch == '-')
				ch = '_';
		}
		boost::to_upper(header);
		header = "HTTP_" + header;
	}


	// Parse Range header
	inline std::pair<std::string, std::string> parse_range(std::string& requested_range)
	{
		std::pair<std::string, std::string> range;
		boost::regex range_regex{ "^bytes=(\\d*)-(\\d*)$" };
		boost::smatch range_match;
		boost::regex_search(requested_range, range_match, range_regex);
		if (!range_match.empty())
		{
			if (range_match[1].first != requested_range.end())
			{
				range.first = std::string{ range_match[1].first, range_match[1].second };
			}
			if (range_match[2].first != requested_range.end())
			{
				range.second = std::string{ range_match[2].first, range_match[2].second };
			}
		}
		return range;
	}
	
	// Get hexadecimal representation of string length
	inline std::string hex(size_t len)
	{
		if (len == 0)
			return "0";
		std::string hex_len;
		while (len > 0)
		{
			hex_len = std::string(1, hex_chars[len % 16]) + hex_len;
			len /= 16;
		}
		return hex_len;
	}


	// Check if a file can be compressed by gzip
	inline bool is_compressable(std::string ext)
	{
		boost::to_lower(ext);
		return std::find(compressables.begin(), compressables.end(), ext) != compressables.end();
	}


	// Determine mime type by file extension
	inline std::string get_mime(std::string ext)
	{
		boost::to_lower(ext);
		try
		{
			return mime_types.at(ext);
		}
		catch (const std::out_of_range&)
		{
			return "application/octet-stream";
		}
	}

#pragma endregion

#pragma region classes

	// RAII implementation for auto-closing an iterator object passed from a WSGI application
	class Iterator
	{
	private:
		boost::python::object m_iterator;

	public:
		explicit Iterator(boost::python::object it) : m_iterator{ it } {}

		~Iterator()
		{
			if (PyObject_HasAttrString(m_iterator.ptr(), "close"))
			{
				m_iterator.attr("close")();
			}
		}

		boost::python::object attr(const std::string& at) const
		{
			return m_iterator.attr(at.c_str());
		}
	};


	// Scoped GIL release
	class GilRelease
	{
	public:
		GilRelease() 
		{
			m_state = PyEval_SaveThread();
		}

		~GilRelease()
		{ 
			PyEval_RestoreThread(m_state);
		}

	private:
		PyThreadState* m_state;
	};


	// Scoped GIL acquire
	class GilAcquire
	{
	public:
		GilAcquire() 
		{
			m_gstate = PyGILState_Ensure();
		}

		~GilAcquire() 
		{
			PyGILState_Release(m_gstate);
		}

	private:
		PyGILState_STATE m_gstate;
	};
}
#pragma endregion
