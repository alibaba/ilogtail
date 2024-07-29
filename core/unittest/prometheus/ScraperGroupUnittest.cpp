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

#include <string>
#include <thread>

#include "JsonUtil.h"
#include "StringTools.h"
#include "prometheus/Labels.h"
#include "prometheus/ScraperGroup.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

// MockHttpClient
class MockScraperHttpClient : public sdk::HTTPClient {
public:
    MockScraperHttpClient();

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

MockScraperHttpClient::MockScraperHttpClient() {
}

void MockScraperHttpClient::Send(const std::string& httpMethod,
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
    if (StartWith(url, "/jobs/")) {
        httpMessage.content = R"JSON({
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
        })JSON";
    }
}

void MockScraperHttpClient::AsynSend(sdk::AsynRequest* request) {
}

class ScraperGroupUnittest : public testing::Test {
public:
    void OnUpdateScrapeJob();
    void OnRemoveScrapeJob();

protected:
    void SetUp() override {
        setenv("POD_NAME", "prometheus-test", 1);
        setenv("OPERATOR_HOST", "127.0.0.1", 1);
        setenv("OPERATOR_PORT", "12345", 1);
    }
    void TearDown() override {
        unsetenv("POD_NAME");
        unsetenv("OPERATOR_HOST");
        unsetenv("OPERATOR_PORT");
    }

private:
};


void ScraperGroupUnittest::OnUpdateScrapeJob() {
    Json::Value config;
    string errorMsg, configStr;
    // start scraper group first
    auto scraperGroup = make_unique<ScraperGroup>();
    scraperGroup->Start();

    configStr = R"JSON({
        "job_name": "test_job",
        "scheme": "http",
        "metrics_path": "/metrics",
        "scrape_interval": "5s",
        "scrape_timeout": "5s"
    })JSON";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));


    // build scrape job and target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    Labels labels;
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});
    scrapeTargets.push_back(ScrapeTarget(labels));

    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>();

    APSARA_TEST_TRUE(scrapeJobPtr->Init(config));
    scrapeJobPtr->mClient.reset(new MockScraperHttpClient());
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].GetHash(), scrapeTargets[0]);

    APSARA_TEST_TRUE(scraperGroup->mScrapeWorkMap.empty());
    scraperGroup->UpdateScrapeJob(std::move(scrapeJobPtr));

    std::this_thread::sleep_for(std::chrono::seconds(6));
    APSARA_TEST_EQUAL((size_t)1,
                      scraperGroup->mScrapeJobMap["test_job"]->GetScrapeTargetsMapCopy().size());
    APSARA_TEST_NOT_EQUAL(nullptr, scraperGroup->mScrapeWorkMap["test_job"][scrapeTargets[0].GetHash()]);
    scraperGroup->RemoveScrapeJob("test_job");

    // stop scraper group to clean up
    scraperGroup->Stop();
}

void ScraperGroupUnittest::OnRemoveScrapeJob() {
    Json::Value config;
    string errorMsg, configStr;
    auto scraperGroup = make_unique<ScraperGroup>();
    // start scraper group first
    scraperGroup->Start();

    // build scrape job and target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    Labels labels;
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});
    scrapeTargets.push_back(ScrapeTarget(labels));

    configStr = R"JSON({
        "job_name": "test_job",
        "scheme": "http",
        "metrics_path": "/metrics",
        "scrape_interval": "5s",
        "scrape_timeout": "5s"
    })JSON";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));

    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>();
    APSARA_TEST_TRUE(scrapeJobPtr->Init(config));
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].GetHash(), scrapeTargets[0]);
    scrapeJobPtr->mClient.reset(new MockScraperHttpClient());

    APSARA_TEST_TRUE(scraperGroup->mScrapeJobMap.empty());
    scraperGroup->UpdateScrapeJob(std::move(scrapeJobPtr));

    // sleep 6s
    std::this_thread::sleep_for(std::chrono::seconds(6));
    APSARA_TEST_EQUAL((size_t)1,
                      scraperGroup->mScrapeJobMap["test_job"]->GetScrapeTargetsMapCopy().size());
    APSARA_TEST_NOT_EQUAL(nullptr, scraperGroup->mScrapeWorkMap["test_job"][scrapeTargets[0].GetHash()]);
    scraperGroup->RemoveScrapeJob("test_job");

    // sleep 1s
    std::this_thread::sleep_for(std::chrono::seconds(1));
    APSARA_TEST_TRUE(scraperGroup->mScrapeJobMap.find("test_job")
                     == scraperGroup->mScrapeJobMap.end());
    APSARA_TEST_TRUE(scraperGroup->mScrapeWorkMap.find("test_job")
                     == scraperGroup->mScrapeWorkMap.end());

    // stop scraper group to clean up
    scraperGroup->Stop();
}

UNIT_TEST_CASE(ScraperGroupUnittest, OnUpdateScrapeJob)
UNIT_TEST_CASE(ScraperGroupUnittest, OnRemoveScrapeJob)

} // namespace logtail

UNIT_TEST_MAIN