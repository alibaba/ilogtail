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

#include "Common.h"
#include "prometheus/PrometheusInputRunner.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

// InputRunnerMockHttpClient
class InputRunnerMockHttpClient : public sdk::HTTPClient {
public:
    void Send(const std::string& httpMethod,
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
    void AsynSend(sdk::AsynRequest* request);
};

void InputRunnerMockHttpClient::Send(const std::string& httpMethod,
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
    if (url.find("/jobs") == 0) {
        httpMessage.content = R"(
    [{
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
    }]
    )";
    }
}

void InputRunnerMockHttpClient::AsynSend(sdk::AsynRequest* request) {
}

class PrometheusInputRunnerUnittest : public testing::Test {
public:
    void OnUpdateScrapeInput();
    void OnRemoveScrapeInput();
    void OnSuccessfulStartAndStop();

protected:
private:
};


void PrometheusInputRunnerUnittest::OnUpdateScrapeInput() {
    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "", 3, 4, map<string, string>()));
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].GetHash(), scrapeTargets[0]);
    scrapeJobPtr->mClient.reset(new InputRunnerMockHttpClient);
    // before
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName")
                     == PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    // 代替插件手动注册scrapeJob
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput("testInputName", std::move(scrapeJobPtr));

    // after
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName")
                     != PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    PrometheusInputRunner::GetInstance()->Stop();
}

void PrometheusInputRunnerUnittest::OnRemoveScrapeInput() {
    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "", 3, 4, map<string, string>()));
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].GetHash(), scrapeTargets[0]);
    scrapeJobPtr->mClient.reset(new InputRunnerMockHttpClient);

    // 代替插件手动注册scrapeJob
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput("testInputName", move(scrapeJobPtr));
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName")
                     != PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    PrometheusInputRunner::GetInstance()->RemoveScrapeInput("testInputName");
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName")
                     == PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    PrometheusInputRunner::GetInstance()->Stop();
}

void PrometheusInputRunnerUnittest::OnSuccessfulStartAndStop() {
    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "", 3, 4, map<string, string>()));
    auto scrapeJobs = std::vector<ScrapeJob>();
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].GetHash(), scrapeTargets[0]);
    scrapeJobPtr->mClient.reset(new InputRunnerMockHttpClient);

    sdk::HTTPClient* httpClient = new InputRunnerMockHttpClient();
    PrometheusInputRunner::GetInstance()->mClient.reset(httpClient);
    // 代替插件手动注册scrapeJob
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput("test_input_name", move(scrapeJobPtr));
    // start
    PrometheusInputRunner::GetInstance()->Start();
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("test_input_name")
                     != PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());

    sleep(5);

    // stop
    PrometheusInputRunner::GetInstance()->Stop();
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("test_input_name")
                     == PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
}

UNIT_TEST_CASE(PrometheusInputRunnerUnittest, OnUpdateScrapeInput)
UNIT_TEST_CASE(PrometheusInputRunnerUnittest, OnRemoveScrapeInput)
UNIT_TEST_CASE(PrometheusInputRunnerUnittest, OnSuccessfulStartAndStop)

} // namespace logtail

UNIT_TEST_MAIN