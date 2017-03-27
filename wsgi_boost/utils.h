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
		std::istringstream ss{ time_string };
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


	// Splits a full path into a path proper and a query string
	inline std::pair<std::string, std::string> split_path(const std::string& path)
	{
		size_t pos = path.find("?");
		if (pos != std::string::npos)
		{
			return std::pair<std::string, std::string>{ path.substr(0, pos), path.substr(pos + 1) };
		}
		return std::pair<std::string, std::string>{ path, "" };
	}


	// Transform a HTTP header to WSGI environ format HTTP_
	inline void transform_header(std::string& header)
	{
		for (auto& ch : header)
		{
			ch = std::toupper(ch);
			if (ch == '-')
				ch = '_';
		}
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
				range.first = std::string{ range_match[1].first, range_match[1].second };
			if (range_match[2].first != requested_range.end())
				range.second = std::string{ range_match[2].first, range_match[2].second };
		}
		return range;
	}
	
	// Get hexadecimal representation of an unisigned int number
	inline std::string hex(size_t u_int)
	{
		if (u_int == 0)
			return "0";
		std::string hex_repr;
		while (u_int > 0)
		{
			hex_repr = std::string(1, hex_chars[u_int % 16]) + hex_repr;
			u_int /= 16;
		}
		return hex_repr;
	}


	// Check if a file can be compressed by gzip
	inline bool is_compressable(const std::string& ext)
	{
		return std::find(compressables.begin(), compressables.end(), boost::to_lower_copy(ext)) != compressables.end();
	}


	// Determine mime type by file extension
	inline std::string get_mime(const std::string& ext)
	{
		try
		{
			return mime_types.at(boost::to_lower_copy(ext));
		}
		catch (const std::out_of_range&)
		{
			return "application/octet-stream";
		}
	}
#pragma endregion

#pragma region classes

	// RAII implementation for auto-closing an iteraterable object passed from a WSGI application
	class Iterable
	{
	public:
		explicit Iterable(boost::python::object it) : m_iterable{ it } {}

		~Iterable()
		{
			if (PyObject_HasAttrString(m_iterable.ptr(), "close"))
			{
				m_iterable.attr("close")();
			}
		}

		boost::python::object attr(const std::string& at) const
		{
			return m_iterable.attr(at.c_str());
		}

		boost::python::ssize_t len() const
		{
			try
			{
				return boost::python::len(m_iterable);
			}
			catch (const boost::python::error_already_set&)
			{
				if (PyErr_ExceptionMatches(PyExc_TypeError) || PyErr_ExceptionMatches(PyExc_AttributeError))
					PyErr_Clear();
				else
					throw;
			}
			return -1;
		}

	private:
		boost::python::object m_iterable;
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


	// wsgi.errors stream implementation
	struct ErrorStream
	{
		void write(std::string msg) { std::cerr << msg; }

		void writelines(boost::python::list lines)
		{
			for (boost::python::ssize_t i = 0; i < boost::python::len(lines); ++i)
			{
				std::string line = boost::python::extract<std::string>(lines[i]);
				std::cerr << line;
			}
		}

		void flush() { std::cerr.flush(); }
	};

	// wsgi.file_wrapper implementation
	class FileWrapper
	{
	private:
		boost::python::object m_file;
		int m_block_size;

	public:
		FileWrapper* call(boost::python::object file, int block_size = 8192)
		{
			m_file = file;
			m_block_size = block_size;
			return this;
		}

		boost::python::object read(int size = -1)
		{
			if (size == -1)
				size = m_block_size;
			return m_file.attr("read")(size);
		}

		FileWrapper* iter() { return this;  }

		boost::python::object next()
		{
			boost::python::object chunk = read();
			std::string str = boost::python::extract<std::string>(chunk);
			if (str.empty())
				throw StopIteration();
			return chunk;
		}

		void close()
		{
			if (PyObject_HasAttrString(m_file.ptr(), "close"))
			{
				m_file.attr("close")();
			}
		}
	};
}
#pragma endregion
