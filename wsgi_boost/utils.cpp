#include "utils.h"

using namespace std;
namespace py = boost::python;

namespace wsgi_boost
{
#pragma region InputWrapper

	string InputWrapper::read(size_t size)
	{
		if (size > 0 && size < m_is.size())
		{
			SafeCharBuffer buffer{ size };
			m_is.read(buffer.data, size);
			return buffer.data;
		}
		return m_is.string();
	}

	string InputWrapper::readline(size_t size)
	{
		SafeCharBuffer buffer{ m_is.size() };
		m_is.getline(buffer.data, m_is.size());
		string ret{ buffer.data };
		if (m_is.good())
			ret += "\n";
		if (size > 0 && ret.length() > size)
			return ret.substr(0, size);
		return ret;
	}

	py::list InputWrapper::readlines(size_t hint)
	{
		py::list listing;
		size_t total_read = 0;
		while (m_is.good())
		{
			listing.append(readline());
			total_read += m_is.gcount();
			if (hint > 0 && total_read >= hint)
				break;
		}
		return listing;
	}

	InputWrapper* InputWrapper::iter()
	{
		return this;
	}

	std::string InputWrapper::next()
	{
		if (m_is.good())
		{
			return readline();
		}
		else
		{
			throw StopIteration();
		}
	}

	size_t InputWrapper::len()
	{
		return m_is.size();
	}

#pragma endregion

#pragma region StringQueue

	void StringQueue::push(string item)
	{
		m_mutex.lock();
		m_queue.push(item);
		m_mutex.unlock();
	}

	string StringQueue::pop()
	{
		std::string tmp;
		m_mutex.lock();
		tmp = m_queue.front();
		m_queue.pop();
		m_mutex.unlock();
		return tmp;
	}

	bool StringQueue::is_empty()
	{
		bool tmp;
		m_mutex.lock();
		tmp = m_queue.empty();
		m_mutex.unlock();
		return tmp;
	}

#pragma endregion

	string time_to_header(time_t posix_time)
	{
		stringstream ss;
		ss.imbue(std::locale("C"));
		ss << put_time(std::gmtime(&posix_time), "%a, %d %b %Y %H:%M:%S GMT");
		return ss.str();
	}


	time_t header_to_time(const string& time_string)
	{
		tm t;
		stringstream ss{ time_string };
		ss.imbue(std::locale("C"));
		ss >> get_time(&t, "%a, %d %b %Y %H:%M:%S GMT");
		return mktime(&t);
	}


	pair<string, string> split_path(const string& path)
	{
		size_t pos = path.find("?");
		if (pos != string::npos)
		{
			return pair<string, string>{path.substr(0, pos), path.substr(pos + 1)};
		}
		return pair<string, string>(path, "");
	}


	string transform_header(string header)
	{
		for (auto& ch : header)
		{
			if (ch == '-')
				ch = '_';
		}
		boost::to_upper(header);
		header = "HTTP_" + header;
		return header;
	}


	string get_current_local_time()
	{
		time_t t = std::time(nullptr);
		stringstream ss;
		ss.imbue(std::locale("C"));
		ss << put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}
}
