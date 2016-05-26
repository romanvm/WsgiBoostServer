#pragma once
/*
Custom exceptions

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include <boost/python.hpp>

#include <string>


namespace wsgi_boost
{
	struct StopIteration : public std::exception {};


	inline void stop_iteration_translator(const StopIteration&)
	{
		PyErr_SetString(PyExc_StopIteration, "Stop!");
	}


	class RuntimeError : public std::exception
	{
	private:
		std::string m_message;

	public:
		RuntimeError(std::string message = "") : m_message{ message } {}

		const char* what() const noexcept
		{
			return m_message.c_str();
		}
	};


	inline void runtime_error_translator(const RuntimeError& ex)
	{
		PyErr_SetString(PyExc_RuntimeError, ex.what());
	}
}
