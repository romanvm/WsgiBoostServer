/*
Boost Python wrapper for the core WSGI/HTTP servers

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "server_http.h"

using namespace boost::python;
using namespace WsgiBoost;


BOOST_PYTHON_MODULE(wsgi_boost)
{
	PyEval_InitThreads(); // Initialize GIL

	scope current;
	current.attr("__doc__") = "This module provides WSGI/HTTP server class";
	current.attr("__version__") = WSGI_BOOST_VERSION;
	current.attr("__author__") = "Roman Miroshnychenko";
	current.attr("__email__") = "romanvm@yandex.ua";
	current.attr("__license__") = "MIT";


	class_<HttpServer, boost::noncopyable>("WsgiBoostHttp",
			"class docstring",
			init<unsigned short, size_t, optional<size_t, size_t>>(args("port", "num_threads", "timeout_request", "timeout_content")))
		.def_readonly("name", &HttpServer::server_name)
		.def("start", &HttpServer::start, "docstring")
		.def("stop", &HttpServer::stop, "docstring")
		.def("add_static_route", &HttpServer::add_static_route, args("path", "content_dir"), "docstring")
		.def("set_app", &HttpServer::set_app, args("app"), "docstring")
		;
}
