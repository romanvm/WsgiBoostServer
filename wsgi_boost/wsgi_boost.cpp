/*
Boost Python wrapper for the core WSGI/HTTP servers

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include "server.h"

namespace py = pybind11;
using namespace wsgi_boost;


PYBIND11_PLUGIN(wsgi_boost)
{
	PyEval_InitThreads(); // Initialize GIL
	
	py::module module{ "wsgi_boost",  "This module provides WSGI/HTTP server class" };
	module.attr("__doc__") = "This module provides WSGI/HTTP server class";
	module.attr("__version__") = WSGI_BOOST_VERSION;
	module.attr("__author__") = "Roman Miroshnychenko";
	module.attr("__email__") = "romanvm@yandex.ua";
	module.attr("__license__") = "MIT";
	py::list all;
	all.append("WsgiBoostHttp");
	module.attr("__all__") = all;
	

	py::class_<HttpServer>(module, "WsgiBoostHttp",
		R"'''(
		WsgiBoostHttp(ip_address='', port=8000, threads=0)

		PEP-3333-compliant multi-threaded WSGI server

		The server can serve both Python WSGI applications and static files.
		For static files gzip compression, 'If-None-Match' and 'If-Modified-Since' headers are supported.

		:param ip_adderess: Server's IP address.
		:type ip_address: str
		:param port: Server's port.
		:type port: int
		:param threads: The number of threads for the server to run.
		    Default: 0 (the number of threads is selected automatically depending on system parameters).
		:type threads: int

		Usage:

		.. code-block:: python

			from wsgi_boost import WsgiBoostHttp

			def hello_app(environ, start_response):
				content = b'Hello World!'
				status = '200 OK'
				response_headers = [('Content-type', 'text/plain'),
									('Content-Length', str(len(content)))]
				start_response(status, response_headers)
				return[content]

			httpd = WsgiBoostHttp(threads=4)
			httpd.set_app(hello_app)
			httpd.add_static_route('^/static', '/var/www/static-files')
			httpd.start()
		)'''")

		.def(py::init<std::string, unsigned short, unsigned int>(),
			py::arg("ip_address") = "", py::arg("port") = 8000, py::arg("threads") = 0)

		.def_property_readonly("is_running", &HttpServer::is_running, "Get server running status")

		.def_readwrite("use_gzip", &HttpServer::use_gzip, "Use gzip compression for static content, default: ``True``")

		.def_readwrite("host_hame", &HttpServer::host_name, "Get or set the host name, default: automatically determined")

		.def_readwrite("header_timeout", &HttpServer::header_timeout,
			R"'''(
			Get or set timeout for receiving HTTP request headers

			This is the max. interval for reciving request headers before closing connection.
			Default: 5s
			)'''")

		.def_readwrite("content_timeout", &HttpServer::content_timeout,
			R"'''(
			Get or set timeout for receiving POST/PUT content or sending response

			This is the max. interval for receiving POST/PUT content
			or sending response before closing connection.
			Default: 300s
			)'''")

		.def_readwrite("url_scheme", &HttpServer::url_scheme,
			"Get or set url scheme -- http or https (default: ``'http'``)"
			)

		.def_readwrite("static_cache_control", &HttpServer::static_cache_control,
			R"'''(
			The value of ``Cache-Control`` HTTP header for static content
			(default: ``'public, max-age=3600'``)
			)'''")

		.def("start", &HttpServer::start,
			R"'''(
			Start processing HTTP requests
			
			.. note:: This method blocks the current thread until the server is stopped
				either by calling :meth:`WsgiBoostHttp.stop` or by pressing :kbd:`Ctrl+C`
			)'''")

		.def("stop", &HttpServer::stop, "Stop processing HTTP requests")

		.def("add_static_route", &HttpServer::add_static_route, py::arg("path"), py::arg("content_dir"),
			R"'''(
			Add a route for serving static files

			Allows to serve static files from ``content_dir``

			:param path: a path regex to match URLs for static files
			:type path: str
			:param content_dir: a directory with static files to be served
			:type content_dir: str

			.. note:: ``path`` parameter is a regex that must start with ``^/``, for example :regexp:`^/static``
				Static URL paths have priority over WSGI application paths,
			    that is, if you set a static route for path :regexp:`^/` *all* requests for ``http://example.com/
			    will be directed to that route and a WSGI application will never be reached.
			)'''")

		.def("set_app", &HttpServer::set_app, py::arg("app"),
			R"'''(
			Set a WSGI application to be served

			:param app: a WSGI application to be served as an executable object
			:type app: object
			:raises RuntimeError: on attempt to set a WSGI application while the server is running
			)'''")
		;


	py::class_<InputStream>(module, "InputStream", "wsgi.input")
		.def("readlines", &InputStream::readlines, py::arg("sizehint") = -1)
		.def("__iter__", &InputStream::iter, py::return_value_policy::reference_internal)
		.def("read", &InputStream::read, py::arg("size") = -1)
		.def("readline", &InputStream::readline, py::arg("size") = -1)
		.def("__next__", &InputStream::next)
		;


	py::class_<ErrorStream>(module, "ErrorStream", "wsgi.errors")
		.def("write", &ErrorStream::write)
		.def("writelines", &ErrorStream::writelines)
		.def("flush", &ErrorStream::flush)
		;


	py::class_<FileWrapper>(module, "FileWrapper", "wsgi.file_wrapper")
		.def("__call__", &FileWrapper::call, py::arg("file"), py::arg("block_size") = 8192,
			py::return_value_policy::reference_internal)
		.def("read", &FileWrapper::read, (py::arg("size") = -1))
		.def("__iter__", &FileWrapper::iter, py::return_value_policy::reference_internal)
		.def("__next__", &FileWrapper::next)
		.def("close", &FileWrapper::close)
		;

	return module.ptr();
}
