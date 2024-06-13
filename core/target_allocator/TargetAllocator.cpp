// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "TargetAllocator.h"
#include <curl/curl.h>
#include "common/StringTools.h"
#include "logger/Logger.h"
namespace logtail {
bool TargetAllocator::Init(const Json::Value& config) {
    this->config = config;
    this->jobName = config["ScrapeConfig"]["job_name"].asString();
    std::vector<std::string> sdConfigs
        = {"azure_sd_configs",       "consul_sd_configs",     "digitalocean_sd_configs", "docker_sd_configs",
           "dockerswarm_sd_configs", "dns_sd_configs",        "ec2_sd_configs",          "eureka_sd_configs",
           "file_sd_configs",        "gce_sd_configs",        "hetzner_sd_configs",      "http_sd_configs",
           "ionos_sd_configs",       "kubernetes_sd_configs", "kuma_sd_configs",         "lightsail_sd_configs",
           "linode_sd_configs",      "marathon_sd_configs",   "nerve_sd_configs",        "nomad_sd_configs",
           "openstack_sd_configs",   "ovhcloud_sd_configs",   "puppetdb_sd_configs",     "scaleway_sd_configs",
           "serverset_sd_configs",   "triton_sd_configs",     "uyuni_sd_configs",        "static_configs"};
    Json::Value httpSDConfig;
    std::string httpPrefix = "http://";
    httpSDConfig["url"] = httpPrefix + ToString(getenv("OPERATOR_HOST")) + ":" + ToString(getenv("OPERATOR_PORT"))
        + "/jobs/" + this->jobName + "/targets?collector_id=" + ToString(getenv("POD_NAME"));
    httpSDConfig["follow_redirects"] = false;
    for (const auto& sdConfig : sdConfigs) {
        this->config["ScrapeConfig"].removeMember(sdConfig);
    }
    this->config["ScrapeConfig"]["http_sd_configs"].append(httpSDConfig);
    return true;
}
bool TargetAllocator::Start() {
    const Json::Value& httpSDConfig = this->config["ScrapeConfig"]["http_sd_configs"][0];
    try {
        this->Refresh(httpSDConfig, this->jobName);
    } catch (const std::exception& e) {
        LOG_WARNING(sLogger,
                    ("http service discovery from operator failed",
                     e.what())("job", this->jobName)("url", httpSDConfig["url"].asString()));
    }
    return true;
}
size_t TargetAllocator::WriteCallback(char* buffer, size_t size, size_t nmemb, std::string* write_buf) {
    unsigned long sizes = size * nmemb;

    if (buffer == NULL) {
        return 0;
    }

    write_buf->append(buffer, sizes);
    return sizes;
}
void TargetAllocator::FetchHttpData(const std::string& url, std::string& readBuffer) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, ("matrix_prometheus_" + ToString(getenv("POD_NAME"))).c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, this->refeshIntervalSeconds);
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(
        headers, ("X-Prometheus-Refresh-Interval-Seconds: " + std::to_string(this->refeshIntervalSeconds)).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (http_code != 200) {
        throw std::runtime_error("Server returned HTTP status code " + std::to_string(http_code));
    }
}
std::vector<std::shared_ptr<TargetAllocator::TargetGroup>>
TargetAllocator::ParseTargetGroups(const std::string& response, const Json::Value& httpSDConfig) {
    Json::CharReaderBuilder readerBuilder;
    JSONCPP_STRING errs;
    Json::Value root;
    std::istringstream s(response);
    if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
        throw std::runtime_error("Failed to parse JSON: " + errs);
    }
    std::vector<std::shared_ptr<TargetGroup>> targetGroups;
    for (const auto& element : root) {
        if (!element.isObject()) {
            throw std::runtime_error("Invalid target group item found");
        }
        auto tg = std::make_shared<TargetGroup>();
        tg->source = httpSDConfig["url"].asString() + ":" + std::to_string(targetGroups.size());
        // Parse labels
        if (element.isMember("labels") && element["labels"].isObject()) {
            for (const auto& labelKey : element["labels"].getMemberNames()) {
                tg->labels[labelKey] = element["labels"][labelKey].asString();
            }
        }
        // Parse targets
        if (element.isMember("targets") && element["targets"].isArray()) {
            for (const auto& target : element["targets"]) {
                if (target.isString()) {
                    tg->targets.push_back(target.asString());
                } else {
                    throw std::runtime_error("Invalid target item found");
                }
            }
        }
        targetGroups.push_back(tg);
    }
    return targetGroups;
}
void TargetAllocator::Refresh(const Json::Value& httpSDConfig, const std::string& jobName) {
    try {
        std::string url = httpSDConfig["url"].asString();
        std::string readBuffer;
        FetchHttpData(url, readBuffer);
        targetGroups = ParseTargetGroups(readBuffer, httpSDConfig);
    } catch (const std::exception& e) {
        throw e;
    }
}
} // namespace logtail