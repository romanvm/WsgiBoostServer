#pragma once

#include <boost/python.hpp>


namespace wsgi_boost
{
	struct StopIteration : public std::exception {};

	void stop_iteration_translator(const StopIteration&)
	{
		PyErr_SetString(PyExc_StopIteration, "Stop!");
	}
}
