#include "server.h"

#include <boost/asio/spawn.hpp>

#include <memory>
#include <signal.h>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;
namespace py = boost::python;

namespace wsgi_boost
{
	HttpServer::HttpServer(std::string ip_address, unsigned short port, unsigned int num_threads) :
		m_ip_address{ ip_address }, m_port{ port }, m_num_threads{ num_threads }, m_acceptor(m_io_service), m_signals{ m_io_service }
	{
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
		asio::spawn(m_io_service, [this, socket](asio::yield_context yc)
		{
			Connection connection{ socket, yc, m_io_service, header_timeout, content_timeout };
			Request request{ connection };
			Response response{ connection };
			if (request.parse_header())
			{
				check_static_route(request);
				response.http_version == request.http_version;
				handle_request(request, response);
			}
			else
			{
				response.send_mesage("400 Bad Request", "Error 400: Bad request!");
			}
		});
	}

	void HttpServer::check_static_route(Request& request)
	{
		for (const auto& route : m_static_routes)
		{
			boost::regex regex{ route.first, boost::regex_constants::icase };
			if (boost::regex_match(request.path, regex))
			{
				request.path_regex = regex;
				request.content_dir = route.second;
				break;
			}
		}
	}

	void HttpServer::handle_request(Request& request, Response& response)
	{
		if (request.content_dir != string())
		{
			request.url_scheme = url_scheme;
			request.host_name = m_host_name;
			request.local_endpoint_port = m_port;
			GilAcquire acquire_gil;
			WsgiRequestHandler handler{ request, response, m_app };
			try
			{
				handler.handle();
			}
			catch (const py::error_already_set&)
			{

			}
			catch (const exception& ex)
			{

			}
		}
		else
		{
			StaticRequestHandler handler{ request, response };
			try
			{
				handler.handle();
			}
			catch (const exception& ex)
			{

			}
		}
		if (request.persistent())
			process_request(request.connection().socket());
	}


	void HttpServer::start()
	{
		GilRelease release_gil;
		if (m_io_service.stopped())
			m_io_service.reset();
		asio::ip::tcp::endpoint endpoint;
		if (m_ip_address != "")
		{
			asio::ip::tcp::resolver resolver(m_io_service);
			endpoint = *resolver.resolve({ m_ip_address, to_string(m_port) });
		}
		else
		{
			endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_port);
		}
		m_acceptor.open(endpoint.protocol());
		m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(reuse_address));
		m_acceptor.bind(endpoint);
		m_acceptor.listen();
		m_host_name = asio::ip::host_name();
		accept();
		for (unsigned int i = 1; i < m_num_threads; ++i)
		{
			m_threads.emplace_back([this]()
			{
				m_io_service.run();
			});
		}
		m_signals.async_wait([this](sys::error_code, int)
		{
			stop();
		});
		m_io_service.run();
		for (auto& t : m_threads)
		{
			t.join();
		}
	}


	void HttpServer::stop()
	{
		m_acceptor.close();
		m_io_service.stop();
		m_signals.cancel();
	}
}
