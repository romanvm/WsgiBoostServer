#pragma once
/*
HTTPS Server class

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/
#ifdef HTTPS_ENABLED

#include "server.h"

// This is needed to link against pre-built OpenSSL from https://slproweb.com/products/Win32OpenSSL.html
#if _MSC_VER && _MSC_VER >= 1900
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }
#endif // end _MSC_VER

#include <boost/asio/ssl.hpp>

namespace wsgi_boost
{
	template<class socket_p>
	class HttpsServer : public BaseServer<socket_p> {};

	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_t;
	typedef std::shared_ptr<ssl_socket_t> ssl_socket_ptr;


	template<>
	class HttpsServer<ssl_socket_ptr> : public BaseServer<ssl_socket_ptr>
	{
	protected:
		boost::asio::ssl::context m_context;

		void accept()
		{
			io_service_ptr io_service = m_io_service_pool.get_io_service();
			ssl_socket_ptr socket = std::make_shared<ssl_socket_t>(*io_service, m_context);
			m_acceptor.async_accept(socket->lowest_layer(),
				[this, io_service, socket](const boost::system::error_code& ec)
			{
				if (ec != boost::asio::error::operation_aborted)
					accept();
				if (!ec)
				{
					socket->lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
					auto timer = std::make_shared<boost::asio::deadline_timer>(*io_service);
					timer->expires_from_now(boost::posix_time::seconds(header_timeout));
					timer->async_wait([socket](const boost::system::error_code& ec)
					{
						if (ec != boost::asio::error::operation_aborted)
						{
							socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
							socket->lowest_layer().close();
						}
					});
					socket->async_handshake(boost::asio::ssl::stream_base::server,
						[this, socket, timer](boost::system::error_code ec)
					{
						timer->cancel();
						if (!ec)
							process_request(socket);
					});

				}
			});
		}

	public:
		HttpsServer(std::string cert, const std::string private_key, std::string dh = std::string(),
			std::string address = std::string(), unsigned short port = 8000, unsigned int threads = 0) :
			// sslv23 is an universal option. Deprecated protocols need to be disabled by set_options()
			// More info: https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_new.html
			m_context{ boost::asio::ssl::context::sslv23_server },
			BaseServer<ssl_socket_ptr>{ address, port, threads }
		{
			url_scheme = "https";
			m_context.set_options(boost::asio::ssl::context::default_workarounds
				| boost::asio::ssl::context::no_sslv2
				| boost::asio::ssl::context::no_sslv3
				| boost::asio::ssl::context::single_dh_use);
			m_context.set_password_callback([](size_t, boost::asio::ssl::context::password_purpose)
			{
				const char* pass = std::getenv("HTTPS_KEY_PASS");
				if (pass)
					return pass;
				return "";
			});
			m_context.use_certificate_chain_file(cert);
			m_context.use_private_key_file(private_key, boost::asio::ssl::context::pem);
			if (!dh.empty())
				m_context.use_tmp_dh_file(dh);
		}

	};
}
#endif // HTTPS_ENABLED
