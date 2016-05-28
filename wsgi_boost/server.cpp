#include "server.h"

#include <boost/asio/spawn.hpp>

#include <memory>
#include <csignal>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;
namespace py = boost::python;


namespace wsgi_boost
{
	HttpServer::HttpServer(std::string ip_address, unsigned short port, unsigned int num_threads) :
		m_ip_address{ ip_address }, m_port{ port }, m_num_threads{ num_threads }, m_acceptor(m_io_service), m_signals{ m_io_service }
	{
		m_is_running.store(false);
		m_signals.add(SIGINT);
		m_signals.add(SIGTERM);
#if defined(SIGQUIT)
		m_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
	}


	void HttpServer::accept()
	{
		socket_ptr socket = make_shared<asio::ip::tcp::socket>(asio::ip::tcp::socket(m_io_service));
		m_acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& ec)
		{
			accept();
			if (!ec)
			{
				socket->set_option(asio::ip::tcp::no_delay(true));
				process_request(socket);
			}
		});
	}


	void HttpServer::process_request(socket_ptr socket)
	{
		// A stackful coroutine is needed here to correctly implement keep-alive
		// in case if the number of concurent requests is greater than
		// the number of server threads.
		// Without the coroutine the next request while all threads are busy
		// hangs in limbo and causes io_service to crash.
		asio::spawn(m_io_service, [this, socket](asio::yield_context yc)
		{
			Connection connection{ socket, m_io_service, yc, header_timeout, content_timeout };
			Request request{ connection };
			Response response{ connection };
			sys::error_code ec = request.parse_header();
			if (!ec)
			{
				check_static_route(request);
				response.http_version = request.http_version;
				handle_request(request, response);
			}
			else if (ec == sys::errc::bad_message)
			{
				response.send_mesage("400 Bad Request");
			}
			else if (ec == sys::errc::invalid_argument)
			{
				response.send_mesage("411 Length Required");
			}
			if (request.persistent())
				process_request(request.connection().socket());
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

	void HttpServer::handle_request(Request& request, Response& response)
	{
		if (request.content_dir == string())
		{
			request.url_scheme = url_scheme;
			request.host_name = host_name;
			request.local_endpoint_port = m_port;
			GilAcquire acquire_gil;
			WsgiRequestHandler handler{ request, response, m_app };
			try
			{
				handler.handle();
			}
			catch (const py::error_already_set&)
			{
				PyErr_Print();
				response.send_mesage("500 Internal Server Error", "Error 500: WSGI application error!");
			}
			catch (const exception& ex)
			{
				cerr << ex.what() << '\n';
				response.send_mesage("500 Internal Server Error", "Error 500: Internal server error!");
			}
		}
		else
		{
			request.use_gzip = use_gzip;
			StaticRequestHandler handler{ request, response };
			try
			{
				handler.handle();
			}
			catch (const exception& ex)
			{
				cerr << ex.what() << '\n';
				response.send_mesage("500 Internal Server Error", "Error 500: Internal server error!");
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
			throw RuntimeError("Attempt to set a WSGI app while the server is running!");
		m_app = app;
	}


	void HttpServer::start()
	{
		if (!is_running())
		{
			cout << "Starting WsgiBoostHttp server...\n";
			GilRelease release_gil;
			if (m_io_service.stopped())
				m_io_service.reset();
			asio::ip::tcp::endpoint endpoint;
			if (m_ip_address != "")
			{
				asio::ip::tcp::resolver resolver(m_io_service);
				try
				{
					endpoint = *resolver.resolve({ m_ip_address, to_string(m_port) });
				}
				catch (const exception&)
				{
					throw RuntimeError("Unable to resolve IP address " + m_ip_address + " and port " + to_string(m_port) + "!");
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
			m_threads.clear();
			for (unsigned int i = 1; i < m_num_threads; ++i)
			{
				m_threads.emplace_back([this]()
				{
					m_io_service.run();
				});
			}
			m_signals.async_wait([this](sys::error_code, int) { stop(); });
			m_is_running.store(true);
			m_io_service.run();
			for (auto& t : m_threads)
			{
				t.join();
			}
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
			m_io_service.stop();
			m_signals.cancel();
			cout << "WsgiBoostHttp server stopped.\n";
			m_is_running.store(false);
		}
	}


	bool HttpServer::is_running() const
	{
		return m_is_running.load();
	}
}
