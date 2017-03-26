#include "io_service_pool.h"

using namespace std;
namespace asio = boost::asio;

namespace wsgi_boost
{
	IoServicePool::IoServicePool(unsigned int ps)
	{
		if (ps > 0)
		{
			pool_size = ps;
		}
		else
		{
			unsigned int threads_hint = thread::hardware_concurrency();
			if (threads_hint > 0)
				pool_size = threads_hint;
			else
				pool_size = 1;
		}
		for (unsigned int i = 0; i < pool_size; ++i)
		{
			// make_shared does not work for io_service
			io_service_ptr io_service{ new asio::io_service };
			auto work = asio::io_service::work{ *io_service };
			m_io_services.emplace_back(io_service);
			m_works.emplace_back(work);
		}
	}


	void IoServicePool::run()
	{
		m_thread_pool.clear();
		for (unsigned int i = 1; i < pool_size; ++i)
		{
			m_thread_pool.emplace_back([this, i]()
			{
				this->m_io_services[i]->run();
			});
		}
		m_io_services[0]->run();
		for (auto& t : m_thread_pool)
			t.join();
	}


	void IoServicePool::stop()
	{
		for (auto& io : m_io_services)
			io->stop();
	}


	void IoServicePool::reset()
	{
		for (auto& io : m_io_services)
		{
			if (io->stopped())
				io->reset();
		}
	}


	io_service_ptr IoServicePool::get_io_service()
	{
		io_service_ptr io_service = m_io_services[m_next_io_service];
		++m_next_io_service;
		if (m_next_io_service == m_io_services.size())
			m_next_io_service = 0;
		return io_service;
	}
}