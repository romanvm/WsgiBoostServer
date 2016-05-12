#include "server.h"

#include <boost/asio/spawn.hpp>
#include <boost/algorithm/string.hpp>

#include <memory>
#include <signal.h>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;

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
				response.http_version == request.http_version;
				if (!send_response(request, response) && request.persistent())
					process_request(socket);
			}
			else
			{
				response.send_mesage("400", "Bad Request");
			}
		});
	}

	sys::error_code HttpServer::send_response(Request& request, Response& response)
	{
		ostringstream oss;
		oss << "Hello World!\n\n";
		oss << "Thread #" << this_thread::get_id() << "\n\n";
		oss << "Method: " << request.method << "\n";
		oss << "Path: " << request.path << "\n\n";
		if (request.get_header("Content-Length") != "")
			oss << "POST content: " << request.post_content() << "\n\n";
		oss << "Request headers:\n";
		for (const auto& item : request.headers)
		{
			oss << item.first << ": " << item.second << "\n";
		}
		headers_type headers;
		headers.emplace_back("Connection", "keep-alive");
		headers.emplace_back("Content-Type", "text/plain");
		headers.emplace_back("Content-Length", to_string(oss.str().length()));
		sys::error_code ec = response.send_header("200", "OK", headers);
		if (!ec)
		{
			ec = response.send_data(oss.str());
		}
		return ec;
	}


	void HttpServer::start()
	{
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
