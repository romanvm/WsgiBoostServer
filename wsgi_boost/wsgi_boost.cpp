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

		"WsgiBoostHttp(port, num_threads=1, timeout_request=5, timeout_content=300)\n\n"

		"A PEP333-compliant WSGI server\n\n"

		"The server is able to serve both Python WSGI applications and static files.\n"
		"For static files gzip compression and 'If-Modified-Since headers are supported.\n\n"

		"Usage::\n\n"

		"	import os\n"
		"	import threading\n"
		"	import time\n"
		"	import wsgi_boost\n\n"

		"	def simple_app(environ, start_response):\n"
		"		content = 'Hello World!\\n\\n'\n"
		"		content += 'POST data: {0}\\n\\n'.format(environ['wsgi.input'].read())\n"
		"		content += 'environ variables:\\n'\n"
		"		for key, value in environ.iteritems():\n"
		"			content += '{0}: {1}\\n'.format(key, value)\n"
		"		status = '200 OK'\n"
		"		response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]\n"
		"		start_response(status, response_headers)\n"
		"		return[content]\n\n"

		"	cwd = os.path.dirname(os.path.abspath(__file__))\n"
		"	httpd = wsgi_boost.WsgiBoostHttp(8000, 4)\n"
		"	httpd.add_static_route('^/static', cwd)\n"
		"	httpd.set_app(simple_app)\n"
		"	server_thread = threading.Thread(target = httpd.start)\n"
		"	server_thread.daemon = True\n"
		"	server_thread.start()\n"
		"	time.sleep(0.5)\n"
		"	try:\n"
		"		while True :\n"
		"			time.sleep(0.1)\n"
		"	except KeyboardInterrupt:\n"
		"		pass\n"
		"	finally:\n"
		"		httpd.stop()\n"
		"		server_thread.join()\n"
		,

		init<unsigned short, optional<size_t, size_t, size_t>>(args("port", "num_threads", "timeout_request", "timeout_content")))

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
			"    will be directed to that route and a WSGI application will never be reached."
		)

		.def("set_app", &HttpServer::set_app, args("app"),
			"Set a WSGI application to be served\n\n"

			":param app: an application to be served as an executable object"
		)
		;
}
