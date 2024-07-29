/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <json/json.h>

#include <iostream>
#include <memory>
#include <string>

#include "ScrapeJob.h"
#include "common/JsonUtil.h"
#include "unittest/Unittest.h"

namespace logtail {

// MockHttpClient
class MockScrapeJobHttpClient : public sdk::HTTPClient {
public:
    MockScrapeJobHttpClient();

    virtual void Send(const std::string& httpMethod,
                      const std::string& host,
                      const int32_t port,
                      const std::string& url,
                      const std::string& queryString,
                      const std::map<std::string, std::string>& header,
                      const std::string& body,
                      const int32_t timeout,
                      sdk::HttpMessage& httpMessage,
                      const std::string& intf,
                      const bool httpsFlag);
    virtual void AsynSend(sdk::AsynRequest* request);
};

MockScrapeJobHttpClient::MockScrapeJobHttpClient() {
}

void MockScrapeJobHttpClient::Send(const std::string& httpMethod,
                                   const std::string& host,
                                   const int32_t port,
                                   const std::string& url,
                                   const std::string& queryString,
                                   const std::map<std::string, std::string>& header,
                                   const std::string& body,
                                   const int32_t timeout,
                                   sdk::HttpMessage& httpMessage,
                                   const std::string& intf,
                                   const bool httpsFlag) {
    httpMessage.statusCode = 200;
    httpMessage.content = R"JSON([
        {
            "targets": [
                "192.168.22.7:8080"
            ],
            "labels": {
                "__meta_kubernetes_pod_controller_kind": "ReplicaSet",
                "__meta_kubernetes_pod_container_image": "registry-vpc.cn-hangzhou.aliyuncs.com/acs/kube-state-metrics:v2.3.0-a71f78c-aliyun",
                "__meta_kubernetes_namespace": "arms-prom",
                "__meta_kubernetes_pod_labelpresent_pod_template_hash": "true",
                "__meta_kubernetes_pod_uid": "00d1897f-d442-47c4-8423-e9bf32dea173",
                "__meta_kubernetes_pod_container_init": "false",
                "__meta_kubernetes_pod_container_port_protocol": "TCP",
                "__meta_kubernetes_pod_host_ip": "192.168.21.234",
                "__meta_kubernetes_pod_controller_name": "kube-state-metrics-64cf88c8f4",
                "__meta_kubernetes_pod_annotation_k8s_aliyun_com_pod_ips": "192.168.22.7",
                "__meta_kubernetes_pod_ready": "true",
                "__meta_kubernetes_pod_node_name": "cn-hangzhou.192.168.21.234",
                "__meta_kubernetes_pod_annotationpresent_k8s_aliyun_com_pod_ips": "true",
                "__address__": "192.168.22.7:8080",
                "__meta_kubernetes_pod_labelpresent_k8s_app": "true",
                "__meta_kubernetes_pod_label_k8s_app": "kube-state-metrics",
                "__meta_kubernetes_pod_container_id": "containerd://57c4dfd8d9ea021defb248dfbc5cc3bd3758072c4529be351b8cc6838bdff02f",
                "__meta_kubernetes_pod_container_port_number": "8080",
                "__meta_kubernetes_pod_ip": "192.168.22.7",
                "__meta_kubernetes_pod_phase": "Running",
                "__meta_kubernetes_pod_container_name": "kube-state-metrics",
                "__meta_kubernetes_pod_container_port_name": "http-metrics",
                "__meta_kubernetes_pod_label_pod_template_hash": "64cf88c8f4",
                "__meta_kubernetes_pod_name": "kube-state-metrics-64cf88c8f4-jtn6v"
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
    ])JSON";
}

void MockScrapeJobHttpClient::AsynSend(sdk::AsynRequest* request) {
}

class ScrapeJobUnittest : public ::testing::Test {
public:
    void OnInitScrapeJob();
    void ScrapeJobTargetsDiscovery();

protected:
    void SetUp() override {
        setenv("POD_NAME", "prometheus-test", 1);
        setenv("OPERATOR_HOST", "127.0.0.1", 1);
        setenv("OPERATOR_PORT", "12345", 1);
        std::string errMsg;
        if (!ParseJsonTable(mConfigString, mConfig, errMsg)) {
            std::cerr << "JSON parsing failed." << std::endl;
        }
    }
    void TearDown() override {
        unsetenv("POD_NAME");
        unsetenv("OPERATOR_HOST");
        unsetenv("OPERATOR_PORT");
    }
    Json::Value mConfig;
    std::string mConfigString = R"JSON(
{
    "Type": "input_prometheus",
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


void ScrapeJobUnittest::OnInitScrapeJob() {
    std::shared_ptr<ScrapeJob> scrapeJobPtr = std::make_shared<ScrapeJob>();
    APSARA_TEST_TRUE(scrapeJobPtr->Init(mConfig["ScrapeConfig"]));
    scrapeJobPtr->mClient.reset(new MockScrapeJobHttpClient);

    APSARA_TEST_NOT_EQUAL(scrapeJobPtr->mScrapeConfigPtr.get(), nullptr);
    APSARA_TEST_EQUAL(scrapeJobPtr->mJobName, "_kube-state-metrics");
}
void ScrapeJobUnittest::ScrapeJobTargetsDiscovery() {
    std::unique_ptr<ScrapeJob> scrapeJobPtr = std::make_unique<ScrapeJob>();
    APSARA_TEST_TRUE(scrapeJobPtr->Init(mConfig["ScrapeConfig"]));

    scrapeJobPtr->mClient.reset(new MockScrapeJobHttpClient);

    scrapeJobPtr->StartTargetsDiscoverLoop();
    std::this_thread::sleep_for(std::chrono::seconds(6));

    const auto& scrapeTargetsMap = scrapeJobPtr->GetScrapeTargetsMapCopy();
    APSARA_TEST_EQUAL(scrapeTargetsMap.size(), 1u);
    for (const auto& [hashTarget, target] : scrapeTargetsMap) {
        APSARA_TEST_TRUE(hashTarget.find("192.168.22.7") != std::string::npos);
        APSARA_TEST_EQUAL(target.mLabels.Size(), 6UL);
        APSARA_TEST_EQUAL(target.mHost, "192.168.22.7");
        APSARA_TEST_EQUAL(target.mPort, 8080UL);
    }

    scrapeJobPtr->StopTargetsDiscoverLoop();
}

UNIT_TEST_CASE(ScrapeJobUnittest, OnInitScrapeJob);
UNIT_TEST_CASE(ScrapeJobUnittest, ScrapeJobTargetsDiscovery);

} // namespace logtail

UNIT_TEST_MAIN