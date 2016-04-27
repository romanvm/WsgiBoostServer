#pragma once
/*
Core HTTP server engine

=====================
The MIT License (MIT)

Copyright (c) 2014 Ole Christian Eidheim
Copyright (c) 2016 Roman Miroshnychenko (modified version)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define WSGI_BOOST_VERSION "0.0.1"

#include "request_handlers.h"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/regex.hpp>

#include <unordered_map>
#include <thread>
#include <functional>
#include <iostream>
#include <sstream>
#include <atomic>
#include <cstdio>


namespace wsgi_boost {
    template <class socket_type>
    class ServerBase {
    public:
		class Response : public std::ostream {
			friend class ServerBase<socket_type>;
		private:
			boost::asio::yield_context& yield;

			boost::asio::streambuf streambuf;

			socket_type &socket;

			Response(socket_type &socket, boost::asio::yield_context& yield) :
				std::ostream(&streambuf), yield(yield), socket(socket) {}

		public:
			size_t size() {
				return streambuf.size();
			}
			void flush() {
				boost::system::error_code ec;
				boost::asio::async_write(socket, streambuf, yield[ec]);

				if (ec)
					throw std::runtime_error(ec.message());
			}
		};

        class Config {
            friend class ServerBase<socket_type>;
        private:
            Config(unsigned short port, size_t num_threads): port(port), num_threads(num_threads), reuse_address(true) {}
            size_t num_threads;
        public:
			unsigned short port;
            ///IPv4 address in dotted decimal form or IPv6 address in hexadecimal notation.
            ///If empty, the address will be any address.
            std::string address;
            ///Set to false to avoid binding the socket to an address that is already in use.
            bool reuse_address;
        };
        ///Set before calling start().
        Config config;
        
		std::vector<std::pair<boost::regex, std::string>> static_routes;
        
    public:
		std::string server_name = "http WsgiBoost Server v." WSGI_BOOST_VERSION;

		std::atomic_bool is_running = false;

		StringQueue msg_queue;

		bool logging = false;

        void start() {
			GilRelease release_gil;

			host_name = boost::asio::ip::host_name();

			if (io_service.stopped())
				io_service.reset();

			boost::asio::ip::tcp::endpoint endpoint;
			if (config.address.size()>0)
				endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(config.address), config.port);
			else
				endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port);
			acceptor.open(endpoint.protocol());
			acceptor.set_option(boost::asio::socket_base::reuse_address(config.reuse_address));
			acceptor.bind(endpoint);
			acceptor.listen();

			accept();

			//If num_threads>1, start m_io_service.run() in (num_threads-1) threads for thread-pooling
			threads.clear();
			for (size_t c = 1; c < config.num_threads; ++c) {
				threads.emplace_back([this]() {
					io_service.run();
				});
			}

			is_running = true;

			std::thread log_thread{ [this]()
				{
					while (this->is_running)
					{
						if (!this->msg_queue.is_empty())
						{
							puts(msg_queue.pop().c_str());
						}
					}
				}
			};

			//Main thread
			io_service.run();

			//Wait for the rest of the threads, if any, to finish as well
			for (auto& t : threads) {
				t.join();
			}

			log_thread.join();
        }
        
        void stop() {
            acceptor.close();
            io_service.stop();
			is_running = false;
        }

		void add_static_route(std::string path, std::string content_dir)
		{
			static_routes.emplace_back(boost::regex(path, boost::regex::icase), content_dir);
		}

		void set_app(boost::python::object app_)
		{
			app = app_;
		}

    protected:
		boost::python::object app;

        boost::asio::io_service io_service;
        boost::asio::ip::tcp::acceptor acceptor;
        std::vector<std::thread> threads;
        
        size_t timeout_request;
        size_t timeout_content;

		std::string host_name;
        
        ServerBase(unsigned short port, size_t num_threads, size_t timeout_request, size_t timeout_send_or_receive) : 
                config(port, num_threads), acceptor(io_service),
                timeout_request(timeout_request), timeout_content(timeout_send_or_receive) {}

		virtual ~ServerBase()
		{
			if (is_running)
				stop();
		}
        
        virtual void accept()=0;

		void handle_wsgi_request(typename ServerBase<socket_type>::Response& response, std::shared_ptr<Request> request)
		{
			request->server_name = server_name;
			request->host_name = host_name;
			request->local_endpoint_port = config.port;
			GilAcquire acquire_gil;
			WsgiRequestHandler request_handler{ response, request, app };
			try
			{
				request_handler.handle_request();
			}
			catch (const boost::python::error_already_set& ex)
			{
				PyErr_Print();
				request_handler.status = "500 Internal Server Error";
				request_handler.send_status("Error 500: WSGI application error!");
			}
			catch (const std::exception& ex)
			{
				request_handler.status = "500 Internal Server Error";
				request_handler.send_status("Error 500: Internal server error!");
			}
			if (logging)
			{
				GilRelease release_gil;
				std::ostringstream ss;
				ss << "[" << get_current_local_time() << "] " << request->method << " " << request->path << " : " << request_handler.status;
				msg_queue.push(ss.str());
			}
		}
        
		void handle_static_request(typename ServerBase<socket_type>::Response& response, std::shared_ptr<Request> request)
		{
			request->server_name = server_name;
			StaticRequestHandler request_handler{ response, request };
			try
			{
				request_handler.handle_request();
			}
			catch (const std::exception& ex)
			{
				request_handler.status = "500 Internal Server Error";
				request_handler.send_status("Error 500: Internal server error while handling static request!");
			}
			if (logging)
			{
				std::ostringstream ss;
				ss << "[" << get_current_local_time() << "] " << request->method << " " << request->path << " : " << request_handler.status;
				msg_queue.push(ss.str());
			}
		}
		

        std::shared_ptr<boost::asio::deadline_timer> set_timeout_on_socket(std::shared_ptr<socket_type> socket, size_t seconds) {
            std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(io_service));
            timer->expires_from_now(boost::posix_time::seconds(seconds));
            timer->async_wait([socket](const boost::system::error_code& ec){
                if(!ec) {
                    boost::system::error_code ec;
                    socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    socket->lowest_layer().close();
                }
            });
            return timer;
        }
        
        std::shared_ptr<boost::asio::deadline_timer> set_timeout_on_socket(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request, size_t seconds) {
            std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(io_service));
            timer->expires_from_now(boost::posix_time::seconds(seconds));
            timer->async_wait(request->strand.wrap([socket](const boost::system::error_code& ec){
                if(!ec) {
                    boost::system::error_code ec;
                    socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    socket->lowest_layer().close();
                }
            }));
            return timer;
        }
        
        void read_request_and_content(std::shared_ptr<socket_type> socket) {
            //Create new streambuf (Request::streambuf) for async_read_until()
            //shared_ptr is used to pass temporary objects to the asynchronous functions
            std::shared_ptr<Request> request(new Request(io_service));
            request->read_remote_endpoint_data(*socket);

            //Set timeout on the following boost::asio::async-read or write function
            std::shared_ptr<boost::asio::deadline_timer> timer;
            if(timeout_request>0)
                timer=set_timeout_on_socket(socket, timeout_request);
                        
            boost::asio::async_read_until(*socket, request->streambuf, "\r\n\r\n",
                    [this, socket, request, timer](const boost::system::error_code& ec, size_t bytes_transferred) {
                if(timeout_request>0)
                    timer->cancel();
                if(!ec) {
                    //request->streambuf.size() is not necessarily the same as bytes_transferred, from Boost-docs:
                    //"After a successful async_read_until operation, the streambuf may contain additional data beyond the delimiter"
                    //The chosen solution is to extract lines from the stream directly when parsing the header. What is left of the
                    //streambuf (maybe some bytes of the content) is appended to in the async_read-function below (for retrieving content).
                    size_t num_additional_bytes=request->streambuf.size()-bytes_transferred;
                    
                    if(!parse_request(request, request->content))
                        return;
                    
                    //If content, read that as well
                    auto it=request->header.find("Content-Length");
                    if(it!=request->header.end()) {
                        //Set timeout on the following boost::asio::async-read or write function
                        std::shared_ptr<boost::asio::deadline_timer> timer;
                        if(timeout_content>0)
                            timer=set_timeout_on_socket(socket, timeout_content);
                        unsigned long long content_length;
                        try {
                            content_length=stoull(it->second);
                        }
                        catch(const std::exception &e) {
                            return;
                        }
                        if(content_length>num_additional_bytes) {
                            boost::asio::async_read(*socket, request->streambuf,
                                    boost::asio::transfer_exactly(content_length-num_additional_bytes),
                                    [this, socket, request, timer]
                                    (const boost::system::error_code& ec, size_t /*bytes_transferred*/) {
                                if(timeout_content>0)
                                    timer->cancel();
                                if(!ec)
                                    find_resource(socket, request);
                            });
                        }
                        else {
                            if(timeout_content>0)
                                timer->cancel();
                            find_resource(socket, request);
                        }
                    }
                    else {
                        find_resource(socket, request);
                    }
                }
            });
        }

        bool parse_request(std::shared_ptr<Request> request, std::istream& stream) const {
            std::string line;
            getline(stream, line);
            size_t method_end;
            if((method_end=line.find(' '))!=std::string::npos) {
                size_t path_end;
                if((path_end=line.find(' ', method_end+1))!=std::string::npos) {
                    request->method=line.substr(0, method_end);
                    request->path=line.substr(method_end+1, path_end-method_end-1);

                    size_t protocol_end;
                    if((protocol_end=line.find('/', path_end+1))!=std::string::npos) {
                        if(line.substr(path_end+1, protocol_end-path_end-1)!="HTTP")
                            return false;
                        request->http_version=line.substr(protocol_end+1, line.size()-protocol_end-2);
                    }
                    else
                        return false;

                    getline(stream, line);
                    size_t param_end;
                    while((param_end=line.find(':'))!=std::string::npos) {
                        size_t value_start=param_end+1;
                        if((value_start)<line.size()) {
                            if(line[value_start]==' ')
                                value_start++;
                            if(value_start<line.size())
                                request->header.insert(std::make_pair(line.substr(0, param_end), line.substr(value_start, line.size()-value_start-1)));
                        }
    
                        getline(stream, line);
                    }
                }
                else
                    return false;
            }
            else
                return false;
            return true;
        }

        void find_resource(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) {
            for(const auto& route: static_routes)
			{                
                boost::smatch sm_res;
                if (boost::regex_search(request->path, sm_res, route.first))
				{
					request->path_regex = route.first;
					request->content_dir = route.second;
                    write_response(socket, request);
                    return;
                }                 
            }
            write_response(socket, request);
        }
        
        void write_response(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) {
            //Set timeout on the following boost::asio::async-read or write function
            std::shared_ptr<boost::asio::deadline_timer> timer;
            if(timeout_content>0)
                timer=set_timeout_on_socket(socket, request, timeout_content);

            boost::asio::spawn(request->strand, [this, socket, request, timer](boost::asio::yield_context yield) {
                Response response(*socket, yield);
                
				if (request->content_dir == "")
				{
					handle_wsgi_request(response, request);
				}
				else
				{
					handle_static_request(response, request);
				}                
                
                if(response.size()>0) {
                    try {
                        response.flush();
                    }
                    catch(const std::exception &e) {
                        return;
                    }
                }
                if(timeout_content>0)
                    timer->cancel();
                float http_version;
                try {
                    http_version=stof(request->http_version);
                }
                catch(const std::exception &e) {
                    return;
                }
                
                auto range=request->header.equal_range("Connection");
                for(auto it=range.first;it!=range.second;it++) {
                    if(boost::iequals(it->second, "close"))
                        return;
                }
                if(http_version>1.05)
                    read_request_and_content(socket);
            });
        }
    };
    
    template<class socket_type>
    class Server : public ServerBase<socket_type> {};
    
    typedef boost::asio::ip::tcp::socket HTTP;
    
    template<>
    class Server<HTTP> : public ServerBase<HTTP> {
    public:
        Server(unsigned short port, size_t num_threads=1, size_t timeout_request=5, size_t timeout_content=300) :
			ServerBase<HTTP>::ServerBase(port, num_threads, timeout_request, timeout_content) {}

    private:
        void accept() {
            //Create new socket for this connection
            //Shared_ptr is used to pass temporary objects to the asynchronous functions
            std::shared_ptr<HTTP> socket(new HTTP(io_service));
                        
            acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& ec){
                //Immediately start accepting a new connection
                accept();
                                
                if(!ec) {
                    boost::asio::ip::tcp::no_delay option(true);
                    socket->set_option(option);
                    
                    read_request_and_content(socket);
                }
            });
        }
    };

	typedef wsgi_boost::Server<wsgi_boost::HTTP> HttpServer;
}
