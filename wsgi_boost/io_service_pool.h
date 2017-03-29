#pragma once
/*
The pool for boost::asio::io_service

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include <boost/asio.hpp>

#include <thread>
#include <memory>
#include <vector>

namespace wsgi_boost
{
	typedef std::shared_ptr<boost::asio::io_service> io_service_ptr;

	class IoServicePool
	{
	private:
		std::vector<io_service_ptr> m_io_services;
		std::vector<boost::asio::io_service::work> m_works;
		std::vector<std::thread> m_threads;
		unsigned int m_next_io_service = 0;

	public:
		explicit IoServicePool(unsigned int pool_size = 0);

		IoServicePool(const IoServicePool&) = delete;
		IoServicePool& operator=(const IoServicePool&) = delete;

		// Run IO services
		void run();

		// Stop IO services
		void stop();

		// Reset IO services
		void reset();

		// Get IO service
		io_service_ptr get_io_service();

		// Get pool size
		size_t size() const;
	};
}
