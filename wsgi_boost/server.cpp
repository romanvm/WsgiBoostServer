#include "server.h"


#include <boost/asio/spawn.hpp>

#include <memory>
#include <csignal>
#include <utility>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;
namespace py = boost::python;


namespace wsgi_boost
{
	HttpServer::HttpServer(std::string ip_address, unsigned short port, unsigned int threads) :
		m_ip_address{ ip_address }, m_port{ port }, m_io_service_pool{ threads },
		m_acceptor{ *(m_io_service_pool.get_io_service()) }, m_signals{ *(m_io_service_pool.get_io_service()) }
	{
		m_is_running.store(false);
		m_signals.add(SIGINT);
		m_signals.add(SIGTERM);
#if defined(SIGQUIT)
		m_signals.add(SIGQUIT);
#endif
	}


	void HttpServer::accept()
	{
		io_service_ptr io_service = m_io_service_pool.get_io_service();
		socket_ptr socket = make_shared<asio::ip::tcp::socket>(asio::ip::tcp::socket(*io_service));
		m_acceptor.async_accept(*socket, [this, io_service, socket](const boost::system::error_code& ec)
		{
			accept();
			if (!ec)
			{
				socket->set_option(asio::ip::tcp::no_delay(true));
				process_request(socket, io_service);
			}
		});
	}


	void HttpServer::process_request(socket_ptr socket, io_service_ptr io_service)
	{
		// A stackful coroutine is needed here to correctly implement keep-alive
		// in case if the number of concurent requests is greater than
		// the number of server threads.
		// Without the coroutine when all threads are busy the next request
		// hangs in limbo and causes io_service to crash.
		asio::spawn(*io_service, [this, socket, io_service](asio::yield_context yc)
		{
			Connection connection{ socket, io_service, yc, header_timeout, content_timeout };
			Request request{ connection };
			Response response{ connection };
			parse_result res = request.parse_header();
			if (!res)
			{
				check_static_route(request);
				response.http_version = request.http_version;
				response.keep_alive = request.keep_alive();
				handle_request(request, response);
			}
			else if (res == BAD_REQUEST)
			{
				response.send_mesage("400 Bad Request", "");
			}
			else if (res == LENGTH_REQUIRED)
			{
				response.send_mesage("411 Length Required", "");
			}
			else
			{
				return;
			}
			// Send all remaining data from the output buffer and re-use the socket
			// for the next request if this is a keep-alive session.
			if (response.keep_alive)
				process_request(socket, io_service);
		});
	}

	void HttpServer::check_static_route(Request& request)
	{
		for (const auto& route : m_static_routes)
		{
			if (boost::regex_search(request.path, route.first))
			{
				request.path_regex = route.first;
				request.content_dir = route.second;
				break;
			}
		}
	}


	void HttpServer::process_error(Response& response, const exception& ex,
		const string& error_msg, bool is_python_error) const
	{
		cerr << error_msg << ": " << ex.what() << '\n';
		if (is_python_error)
			PyErr_Print();
		if (!response.header_sent())
		{
			response.clear();
			response.send_html("500 Internal Server Error", "Error 500", "Internal Server Error", error_msg);
		}
		else
			// Do not reuse a socket on a fatal error
			response.keep_alive = false;
	}


	void HttpServer::handle_request(Request& request, Response& response)
	{
		if (request.content_dir.empty())
		{
			// Try to buffer the first 128KB of request data
			if (request.connection().post_content_length() > 0)
			{
				sys::error_code ec;
				if (request.check_header("Expect", "100-continue"))
					// Send only plain status string with no headers
					ec = response.send_data("HTTP/1.1 100 Continue\r\n\r\n");
				if (ec || !request.connection().read_into_buffer(min(request.connection().post_content_length(), 131072LL), true))
				{
					cerr << "Unable to buffer POST/PUT/PATCH data from " << request.remote_address() << ':' << request.remote_port() << '\n';
					response.keep_alive = false;
					return;
				}
			}
			GilAcquire acquire_gil;
			WsgiRequestHandler handler{ request, response, m_app, url_scheme, host_name, m_port, m_io_service_pool.size() > 1 };
			try
			{
				handler.handle();
			}
			catch (const py::error_already_set&)
			{
				process_error(response, runtime_error(""), "Python error while processing a WSGI request", true);
			}
			catch (const exception& ex)
			{
				process_error(response, ex, "General error while processing a WSGI request");
			}
		}
		else
		{
			StaticRequestHandler handler{ request, response, static_cache_control, use_gzip };
			try
			{
				handler.handle();
			}
			catch (const exception& ex)
			{
				process_error(response, ex, "Error while processing a static request");
			}
		}
	}


	void HttpServer::add_static_route(string path, string content_dir)
	{
		m_static_routes.emplace_back(boost::regex(path, boost::regex_constants::icase), content_dir);
	}


	void HttpServer::set_app(py::object app)
	{
		if (is_running())
			throw runtime_error("Cannot set a WSGI app while the server is running!");
		m_app = app;
	}


	void HttpServer::start()
	{
		if (!is_running())
		{
			GilRelease release_gil;
			cout << "WsgiBoostHttp server starting with " << m_io_service_pool.size() << " thread(s).\n";
			cout << "Press Ctrl+C to stop it.\n";
			m_io_service_pool.reset();
			asio::ip::tcp::endpoint endpoint;
			if (m_ip_address != "")
			{
				asio::ip::tcp::resolver resolver(*(m_io_service_pool.get_io_service()));
				try
				{
					endpoint = *resolver.resolve({ m_ip_address, to_string(m_port) });
				}
				catch (const exception&)
				{
					throw runtime_error("Unable to resolve IP address and port " + m_ip_address + ":" + to_string(m_port) + "!");
				}
			}
			else
			{
				endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_port);
			}
			m_acceptor.open(endpoint.protocol());
			m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(reuse_address));
			m_acceptor.bind(endpoint);
			m_acceptor.listen();
			if (host_name == string())
				host_name = asio::ip::host_name();
			accept();
			m_signals.async_wait([this](sys::error_code, int) { stop(); });
			m_is_running.store(true);
			m_io_service_pool.run();
			m_is_running.store(false);
			cout << "WsgiBoostHttp server stopped.\n";
		}
		else
		{
			cerr << "The server is already running!\n";
		}
	}


	void HttpServer::stop()
	{
		if (is_running())
		{
			m_acceptor.close();
			m_io_service_pool.stop();
			m_signals.cancel();
		}
		else
		{
			cerr << "The server is not running!\n";
		}
	}


	bool HttpServer::is_running() const
	{
		return m_is_running.load();
	}
}
