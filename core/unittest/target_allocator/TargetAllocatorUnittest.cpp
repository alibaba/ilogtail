#include <curl/curl.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "TargetAllocator.h"
#include "unittest/Unittest.h"
namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>
namespace logtail {
class TargetAllocatorUnittest : public ::testing::Test {
public:
    void TestTargetAllocatorInit();
    void TestTargetAllocatorStart();
protected:
    void SetUp() override {
        setenv("POD_NAME", "matrix-test", 1);
        setenv("OPERATOR_HOST", "127.0.0.1", 1);
        setenv("OPERATOR_PORT", "12345", 1);
        if (StringToJsonValue(configString, config)) {
            std::cout << "JSON parsing succeeded." << std::endl;
            std::cout << config.toStyledString() << std::endl;
        } else {
            std::cerr << "JSON parsing failed." << std::endl;
        }
    }
    void TearDown() override {
        unsetenv("POD_NAME");
        unsetenv("OPERATOR_HOST");
        unsetenv("OPERATOR_PORT");
    }
    bool StringToJsonValue(const std::string& jsonString, Json::Value& jsonValue) {
        Json::CharReaderBuilder readerBuilder;
        std::string errs;
        std::istringstream iss(jsonString);
        bool parsingSuccessful = Json::parseFromStream(readerBuilder, iss, &jsonValue, &errs);
        if (!parsingSuccessful) {
            std::cerr << "Failed to parse JSON: " << errs << std::endl;
        }
        return parsingSuccessful;
    }
    Json::Value config;
    std::string configString = R"JSON(
{
    "Type": "matrix_prometheus",
    "ScrapeConfig": {
        "enable_http2": true,
        "follow_redirects": true,
        "honor_timestamps": false,
        "job_name": "_kube-state-metrics",
        "kubernetes_sd_configs": [
            {
                "enable_http2": true,
                "follow_redirects": true,
                "kubeconfig_file": "",
                "namespaces": {
                    "names": [
                        "arms-prom"
                    ],
                    "own_namespace": false
                },
                "role": "pod"
            }
        ],
        "metrics_path": "/metrics",
        "relabel_configs": [
            {
                "action": "keep",
                "regex": "kube-state-metrics",
                "replacement": "$1",
                "separator": ";",
                "source_labels": [
                    "__meta_kubernetes_pod_label_k8s_app"
                ]
            },
            {
                "action": "keep",
                "regex": "8080",
                "replacement": "$1",
                "separator": ";",
                "source_labels": [
                    "__meta_kubernetes_pod_container_port_number"
                ]
            },
            {
                "action": "replace",
                "regex": "([^:]+)(?::\\d+)?;(\\d+)",
                "replacement": "$1:$2",
                "separator": ";",
                "source_labels": [
                    "__address__",
                    "__meta_kubernetes_pod_container_port_number"
                ],
                "target_label": "__address__"
            }
        ],
        "scheme": "http",
        "scrape_interval": "30s",
        "scrape_timeout": "30s"
    }
}
    )JSON";
};
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
        std::unordered_map<std::string, std::string> response_map = {
            {"/jobs/_kube-state-metrics/targets?collector_id=matrix-test", R"JSON([
    {
        "targets": [
            "192.168.22.32:6443"
        ],
        "labels": {
            "__meta_kubernetes_endpoints_labelpresent_endpointslice_kubernetes_io_skip_mirror": "true",
            "__meta_kubernetes_endpoint_port_name": "https",
            "__meta_kubernetes_endpoint_ready": "true",
            "__address__": "192.168.22.32:6443",
            "__meta_kubernetes_namespace": "default",
            "__meta_kubernetes_service_label_component": "apiserver",
            "__meta_kubernetes_endpoints_label_endpointslice_kubernetes_io_skip_mirror": "true",
            "__meta_kubernetes_endpoint_port_protocol": "TCP",
            "__meta_kubernetes_service_labelpresent_provider": "true",
            "__meta_kubernetes_service_name": "kubernetes",
            "__meta_kubernetes_endpoints_name": "kubernetes",
            "__meta_kubernetes_service_labelpresent_component": "true",
            "__meta_kubernetes_service_label_provider": "kubernetes"
        }
    },
    {
        "targets": [
            "192.168.22.31:6443"
        ],
        "labels": {
            "__address__": "192.168.22.31:6443",
            "__meta_kubernetes_endpoint_port_protocol": "TCP",
            "__meta_kubernetes_service_label_provider": "kubernetes",
            "__meta_kubernetes_endpoints_name": "kubernetes",
            "__meta_kubernetes_service_name": "kubernetes",
            "__meta_kubernetes_endpoints_labelpresent_endpointslice_kubernetes_io_skip_mirror": "true",
            "__meta_kubernetes_service_labelpresent_provider": "true",
            "__meta_kubernetes_endpoint_port_name": "https",
            "__meta_kubernetes_namespace": "default",
            "__meta_kubernetes_service_label_component": "apiserver",
            "__meta_kubernetes_service_labelpresent_component": "true",
            "__meta_kubernetes_endpoint_ready": "true"
        }
    }
])JSON"},
        };
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
UNIT_TEST_CASE(TargetAllocatorUnittest, TestTargetAllocatorInit);
UNIT_TEST_CASE(TargetAllocatorUnittest, TestTargetAllocatorStart);
void TargetAllocatorUnittest::TestTargetAllocatorInit() {
    bool result = TargetAllocator::GetInstance()->Init(config);
    EXPECT_TRUE(result);
    const Json::Value& newConfig = TargetAllocator::GetInstance()->GetConfig();
    EXPECT_TRUE(newConfig["ScrapeConfig"].isMember("http_sd_configs"));
    EXPECT_TRUE(newConfig["ScrapeConfig"]["http_sd_configs"].isArray());
    EXPECT_EQ(newConfig["ScrapeConfig"]["http_sd_configs"].size(), 1u);
    for (const auto& httpSdConfig : newConfig["ScrapeConfig"]["http_sd_configs"]) {
        EXPECT_TRUE(httpSdConfig.isMember("url"));
        EXPECT_EQ(httpSdConfig["url"].asString(),
                  "http://" + std::string(getenv("OPERATOR_HOST")) + ":" + std::string(getenv("OPERATOR_PORT"))
                      + "/jobs/" + TargetAllocator::GetInstance()->GetJobName()
                      + "/targets?collector_id=" + std::string(getenv("POD_NAME")));
    }
}
void TargetAllocatorUnittest::TestTargetAllocatorStart() {
    std::thread server_thread([]() { start_server(); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    bool initResult = TargetAllocator::GetInstance()->Init(config);
    EXPECT_TRUE(initResult);
    const Json::Value& newConfig = TargetAllocator::GetInstance()->GetConfig();
    std::cout << newConfig.toStyledString() << std::endl;
    bool result = false;
    try {
        result = TargetAllocator::GetInstance()->Start();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    const std::vector<std::shared_ptr<logtail::TargetAllocator::TargetGroup>>& targetGroup
        = TargetAllocator::GetInstance()->GetTargets();
    EXPECT_EQ(targetGroup.size(), 2u);
    EXPECT_EQ(targetGroup[0].get()->targets.size(), 1u);
    EXPECT_EQ(targetGroup[0].get()->targets[0], "192.168.22.32:6443");
    EXPECT_EQ(targetGroup[0].get()->labels.size(), 13u);
    EXPECT_EQ(targetGroup[1].get()->targets.size(), 1u);
    EXPECT_EQ(targetGroup[1].get()->targets[0], "192.168.22.31:6443");
    EXPECT_EQ(targetGroup[1].get()->labels.size(), 12u);
    EXPECT_TRUE(result);
    server_thread.detach();
}
} // namespace logtail
UNIT_TEST_MAIN