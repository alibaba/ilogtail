#pragma once

#include <curl/curl.h>
#include <json/json.h>

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace logtail {
http::response<http::string_body> handle_request(const http::request<http::string_body>& req,
                                                 const std::unordered_map<std::string, std::string>& response_map);
void do_session(tcp::socket socket, const std::unordered_map<std::string, std::string>& response_map);
void start_server();
} // namespace logtail
