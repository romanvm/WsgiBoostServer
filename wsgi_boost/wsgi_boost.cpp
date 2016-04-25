/*
Boost Python wrapper for the core WSGI/HTTP servers

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "server_http.h"

using namespace boost::python;
using namespace wsgi_boost;


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

		.def_readonly("name", &HttpServer::server_name, "Server name")

		.def("start", &HttpServer::start, "Start processing HTTP requests")

		.def("stop", &HttpServer::stop, "Stop processing HTTP requests")

		.def("add_static_route", &HttpServer::add_static_route, args("path", "content_dir"),

			"Add a route for serving static files\n\n"

			"Allows to serve static files from ``content_dir``\n\n"

			":param path: a path regex to match URLs for static files\n"
			":type path: str\n"
			":param content_dir: a directory with static files to be served\n"
			":type content_dir: str\n\n"

			".. note:: ``path`` parameter is a regex that must start with ``^/``, for example :regexp:`^/static``\n\n"

			".. warning:: static URL paths have priority over WSGI application paths,\n"
			"    that is, if you set a static route for path :regexp:`^/` *all* requests for ``http://example.com/ \n"
			"    will be directed to that route and a WSGI application will never be reached.\n\n"
		)

		.def("set_app", &HttpServer::set_app, args("app"), "docstring")
		;
}
