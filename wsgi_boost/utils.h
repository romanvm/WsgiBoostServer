#pragma once

#include<string>
#include <sstream>
#include <ctime>
#include <iomanip>


std::string time_to_header(std::time_t posix_time)
{
	std::stringstream ss;
	ss.imbue(std::locale("C"));
	ss << std::put_time(std::gmtime(&posix_time), "%a, %d %b %Y %H:%M:%S GMT");
	return ss.str();
}


std::time_t header_to_time(const std::string& time_string)
{
	std::tm t;
	std::stringstream ss{ time_string };
	ss.imbue(std::locale("C"));
	ss >> std::get_time(&t, "%a, %d %b %Y %H:%M:%S GMT");
	return std::mktime(&t);
}


std::string get_current_gmt_time()
{
	return time_to_header(std::time(nullptr));
}
