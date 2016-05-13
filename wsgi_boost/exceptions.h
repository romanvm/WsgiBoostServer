#pragma once
/*
Custom exceptions

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include <boost/python.hpp>


namespace wsgi_boost
{
	struct StopIteration : public std::exception {};

	inline void stop_iteration_translator(const StopIteration&)
	{
		PyErr_SetString(PyExc_StopIteration, "Stop!");
	}
}
