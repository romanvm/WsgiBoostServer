#pragma once

#include <boost/python.hpp>

#include <string>

namespace wsgi_boost
{
	struct StopIteration : public std::exception {};

	void stop_iteration_translator(const StopIteration& ex)
	{
		PyErr_SetString(PyExc_StopIteration, "Stop!");
	}
}
