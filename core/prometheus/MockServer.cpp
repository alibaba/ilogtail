#include "MockServer.h"

#include <iostream>

using namespace std;

namespace logtail {
http::response<http::string_body> handle_request(const http::request<http::string_body>& req,
                                                 const std::unordered_map<std::string, std::string>& response_map) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    auto it = response_map.find(std::string(req.target()));
    if (it != response_map.end()) {
        res.body() = it->second;
    } else {
        res.result(http::status::not_found);
        res.body() = "Resource not found";
    }
    res.prepare_payload();
    return res;
}
void do_session(tcp::socket socket, const std::unordered_map<std::string, std::string>& response_map) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);
        http::response<http::string_body> res = handle_request(req, response_map);
        http::write(socket, res);
        socket.shutdown(tcp::socket::shutdown_send);
    } catch (std::exception const& e) {
        std::cerr << "Error in session: " << e.what() << std::endl;
    }
}
void start_server() {
    try {
        auto const address = net::ip::make_address(getenv("OPERATOR_HOST"));
        unsigned short port = static_cast<unsigned short>(std::stoi(getenv("OPERATOR_PORT")));
        std::cout << "Server starting at " << address.to_string() << ":" << port << std::endl;
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};
        std::unordered_map<std::string, std::string> response_map
            = {{"/jobs/_kube-state-metrics/targets?collector_id=" + string(std::getenv("POD_NAME")), R"JSON([
    {
        "targets": [
            "127.0.0.1:9100"
        ],
        "labels": {
            "__address__": "127.0.0.1:9100",
        }
    }
])JSON"},
               {"register_collector?pod_name=" + string(std::getenv("POD_NAME")),
                R"JSON({"unRegisterMs":"1719994213084"})JSON"}};
        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread{[&response_map](tcp::socket socket) { do_session(std::move(socket), response_map); },
                        std::move(socket)}
                .detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

} // namespace logtail
