#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "server_http.h"

#include "catch.h"

using namespace std;
using namespace WsgiBoost;


TEST_CASE("Test StaticRequestHandler", "[static_files]")
{
	// Test data
	std::stringstream response;
	const std::string server_name = "Test Server";
	std::string http_version = "1.1";
	std::string method;
	std::string path = "/";
	std::string content_dir;
	std::unordered_multimap<std::string, std::string, ihash, iequal_to> in_headers;
	boost::regex path_regex;

	SECTION("Test accepting only GET and HEAD methods")
	{
		method = "POST";
		std::string content_dir = boost::filesystem::current_path().string();
		StaticRequestHandler handler{response, server_name, http_version, method, path, content_dir, in_headers, path_regex};
		handler.handle_request();
		string result = response.str();
		REQUIRE( result.find("405 Method Not Allowed") != string::npos );
	}

	SECTION("Test sending 500 error if a static content dir does not exist")
	{
		method = "GET";
		content_dir = "/foo/bar/baz";
		StaticRequestHandler handler{ response, server_name, http_version, method, path, content_dir, in_headers, path_regex };
		handler.handle_request();
		string result = response.str();
		REQUIRE(result.find("500 Internal Server Error") != string::npos);
	}
}
