#pragma once
/*
Custom exceptions

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include <cmath> // Needed to compile with MinGW
#include <boost/python.hpp>

#include <string>


namespace wsgi_boost
{
	struct StopIteration : public std::exception {};


	inline void stop_iteration_translator(const StopIteration&)
	{
		PyErr_SetString(PyExc_StopIteration, "Stop!");
	}


	inline void runtime_error_translator(const std::runtime_error& ex)
	{
		PyErr_SetString(PyExc_RuntimeError, ex.what());
	}
}
