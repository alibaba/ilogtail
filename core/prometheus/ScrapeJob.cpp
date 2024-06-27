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

#include "ScrapeJob.h"

#include <curl/curl.h>

#include "common/FileSystemUtil.h"
#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"

using namespace std;

namespace logtail {

const int sRefeshIntervalSeconds = 5;

string url_encode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (char c : value) {
        // Keep alphanumeric characters and other safe characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        // Any other characters are percent-encoded
        escaped << '%' << setw(2) << int((unsigned char)c);
    }

    return escaped.str();
}

/// @brief Construct from json config
ScrapeJob::ScrapeJob(const Json::Value& scrapeConfig) {
    mScrapeConfig = scrapeConfig;
    if (mScrapeConfig.isMember("job_name")) {
        mJobName = mScrapeConfig["job_name"].asString();
    }
    if (mScrapeConfig.isMember("scheme")) {
        mScheme = mScrapeConfig["scheme"].asString();
    } else {
        mScheme = "http";
    }
    if (mScrapeConfig.isMember("metrics_path")) {
        mMetricsPath = mScrapeConfig["metrics_path"].asString();
    } else {
        mMetricsPath = "/metrics";
    }
    if (mScrapeConfig.isMember("scrape_interval")) {
        mScrapeIntervalString = mScrapeConfig["scrape_interval"].asString();
    } else {
        mScrapeIntervalString = "30s";
    }
    if (mScrapeConfig.isMember("scrape_timeout")) {
        mScrapeTimeoutString = mScrapeConfig["scrape_timeout"].asString();
    } else {
        mScrapeTimeoutString = "10s";
    }
    if (mScrapeConfig.isMember("params") && mScrapeConfig["params"].isObject()) {
        const Json::Value& params = mScrapeConfig["params"];
        if (params.isObject()) {
            for (const auto& key : params.getMemberNames()) {
                const Json::Value& values = params[key];
                if (values.isArray()) {
                    vector<string> valueList;
                    for (const auto& value : values) {
                        valueList.push_back(value.asString());
                    }
                    mParams[key] = valueList;
                }
            }
        }
    }
    if (mScrapeConfig.isMember("bearer_token_file")) {
        string bearerToken;
        bool b = ReadFile(mScrapeConfig["bearer_token_file"].asString(), bearerToken);
        if (!b) {
            LOG_ERROR(sLogger, ("read bearer_token_file failed, bearer_token_file", mScrapeConfig["bearer_token_file"].asString()));
        }
        string mode = " Bearer ";
        mHeaders["Authorization"] = mode + bearerToken;
    }
    vector<string> sdConfigs
        = {"azure_sd_configs",       "consul_sd_configs",     "digitalocean_sd_configs", "docker_sd_configs",
           "dockerswarm_sd_configs", "dns_sd_configs",        "ec2_sd_configs",          "eureka_sd_configs",
           "file_sd_configs",        "gce_sd_configs",        "hetzner_sd_configs",      "http_sd_configs",
           "ionos_sd_configs",       "kubernetes_sd_configs", "kuma_sd_configs",         "lightsail_sd_configs",
           "linode_sd_configs",      "marathon_sd_configs",   "nerve_sd_configs",        "nomad_sd_configs",
           "openstack_sd_configs",   "ovhcloud_sd_configs",   "puppetdb_sd_configs",     "scaleway_sd_configs",
           "serverset_sd_configs",   "triton_sd_configs",     "uyuni_sd_configs",        "static_configs"};
    Json::Value httpSDConfig;
    string httpPrefix = "http://";
    httpSDConfig["url"] = httpPrefix + ToString(getenv("OPERATOR_HOST")) + ":" + ToString(getenv("OPERATOR_PORT"))
        + "/jobs/" + url_encode(mJobName) + "/targets?collector_id=" + ToString(getenv("POD_NAME"));
    httpSDConfig["follow_redirects"] = false;
    for (const auto& sdConfig : sdConfigs) {
        mScrapeConfig.removeMember(sdConfig);
    }
    mScrapeConfig["http_sd_configs"].append(httpSDConfig);
    for (const auto& relabelConfig : mScrapeConfig["relabel_configs"]) {
        mRelabelConfigs.push_back(RelabelConfig(relabelConfig));
    }
}


bool ScrapeJob::isValid() const {
    return !mJobName.empty();
}

/// @brief Sort temporarily by name
bool ScrapeJob::operator<(const ScrapeJob& other) const {
    return mJobName < other.mJobName;
}

void ScrapeJob::StartTargetsDiscoverLoop() {
    mFinished.store(false);
    if (!mTargetsDiscoveryLoopThread) {
        mTargetsDiscoveryLoopThread = CreateThread([this]() { TargetsDiscoveryLoop(); });
    }
}

void ScrapeJob::StopTargetsDiscoverLoop() {
    mFinished.store(true);
    mTargetsDiscoveryLoopThread->Wait(0ULL);
    mTargetsDiscoveryLoopThread.reset();
    mScrapeTargetsMap.clear();
}

unordered_map<string, unique_ptr<ScrapeTarget>> ScrapeJob::GetScrapeTargetsMapCopy() {
    lock_guard<mutex> lock(mMutex);

    std::unordered_map<std::string, std::unique_ptr<ScrapeTarget>> copy;

    for (const auto& pair : mScrapeTargetsMap) {
        copy[pair.first] = std::make_unique<ScrapeTarget>(*pair.second);
    }

    return copy;
}

void ScrapeJob::TargetsDiscoveryLoop() {
    uint64_t nextTargetsDiscoveryLoopTime = GetCurrentTimeInNanoSeconds();
    while (!mFinished.load()) {
        uint64_t timeNow = GetCurrentTimeInNanoSeconds();
        if (timeNow < nextTargetsDiscoveryLoopTime) {
            this_thread::sleep_for(chrono::nanoseconds(nextTargetsDiscoveryLoopTime - timeNow));
        }
        nextTargetsDiscoveryLoopTime
            = GetCurrentTimeInNanoSeconds() + sRefeshIntervalSeconds * 1000ULL * 1000ULL * 1000ULL;

        string url = mScrapeConfig["http_sd_configs"][0]["url"].asString();
        string readBuffer;
        bool b1 = FetchHttpData(url, readBuffer);
        if (!b1) {
            continue;
        }
        unordered_map<string, unique_ptr<ScrapeTarget>> newScrapeTargetsMap
            = unordered_map<string, unique_ptr<ScrapeTarget>>();
        bool b2 = ParseTargetGroups(readBuffer, url, newScrapeTargetsMap);
        if (!b2) {
            continue;
        }
        mScrapeTargetsMap = move(newScrapeTargetsMap);
    }
}

size_t ScrapeJob::WriteCallback(char* buffer, size_t size, size_t nmemb, string* write_buf) {
    unsigned long sizes = size * nmemb;

    if (buffer == NULL) {
        return 0;
    }

    write_buf->append(buffer, sizes);
    return sizes;
}

bool ScrapeJob::FetchHttpData(const string& url, string& readBuffer) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR(
            sLogger,
            ("http service discovery from operator failed", "Failed to initialize CURL")("job", mJobName)("url", url));
        return false;
    }
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, ("matrix_prometheus_" + ToString(getenv("POD_NAME"))).c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, sRefeshIntervalSeconds);
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(
        headers, ("X-Prometheus-Refresh-Interval-Seconds: " + to_string(sRefeshIntervalSeconds)).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        LOG_ERROR(sLogger,
                  ("http service discovery from operator failed",
                   "CURL request failed: " + string(curl_easy_strerror(res)))("job", mJobName)("url", url));
        return false;
    }
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (http_code != 200) {
        LOG_ERROR(sLogger,
                  ("http service discovery from operator failed",
                   "Server returned HTTP status code " + to_string(http_code))("job", mJobName)("url", url));
        return false;
    }
    return true;
}
bool ScrapeJob::ParseTargetGroups(const string& response,
                                  const string& url,
                                  unordered_map<string, unique_ptr<ScrapeTarget>>& newScrapeTargetsMap) const {
    Json::CharReaderBuilder readerBuilder;
    JSONCPP_STRING errs;
    Json::Value root;
    istringstream s(response);
    if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
        LOG_ERROR(sLogger,
                  ("http service discovery from operator failed",
                   "Failed to parse JSON: " + errs)("job", mJobName)("url", url));
        return false;
    }
    for (const auto& element : root) {
        if (!element.isObject()) {
            LOG_ERROR(sLogger,
                      ("http service discovery from operator failed",
                       "Invalid target group item found")("job", mJobName)("url", url));
            return false;
        }

        string source = url + ":" + to_string(newScrapeTargetsMap.size());

        // Parse targets
        vector<string> targets;
        if (element.isMember("targets") && element["targets"].isArray()) {
            for (const auto& target : element["targets"]) {
                if (target.isString()) {
                    targets.push_back(target.asString());
                } else {
                    LOG_ERROR(sLogger,
                              ("http service discovery from operator failed",
                               "Invalid target item found")("job", mJobName)("url", url));
                    return false;
                }
            }
        }

        // Parse labels
        Labels labels;
        labels.push_back(Label{"job", mJobName});
        labels.push_back(Label{"__address__", targets[0]});
        labels.push_back(Label{"__scheme__", mScheme});
        labels.push_back(Label{"__metrics_path__", mMetricsPath});
        labels.push_back(Label{"__scrape_interval__", mScrapeIntervalString});
        labels.push_back(Label{"__scrape_timeout__", mScrapeTimeoutString});
        for (const auto& pair : mParams) {
            labels.push_back(Label{"__param_" + pair.first, pair.second[0]});
        }

        if (element.isMember("labels") && element["labels"].isObject()) {
            for (const auto& labelKey : element["labels"].getMemberNames()) {
                labels.push_back(Label{labelKey, element["labels"][labelKey].asString()});
            }
        }

        // Relabel Config
        Labels result;
        bool keep = relabel::Process(labels, mRelabelConfigs, result);
        if (!keep) {
            continue;
        }
        result.RemoveMetaLabels();
        if (result.size() == 0) {
            continue;
        }

        ScrapeTarget st = ScrapeTarget(targets, result, source);
        st.mJobName = mJobName;
        st.mScheme = mScheme;
        st.mMetricsPath = mMetricsPath;
        st.mHeaders = mHeaders;
        st.mScrapeInterval = GetIntSeconds(mScrapeIntervalString);
        st.mScrapeTimeout = GetIntSeconds(mScrapeTimeoutString);

        bool b = BuildScrapeURL(result, st);
        if (!b) {
            continue;
        }

        st.mQueryString = ConvertMapParamsToQueryString();

        newScrapeTargetsMap[st.mHash] = make_unique<ScrapeTarget>(st);
    }
    return true;
}

// only support xxs, xxm
int ScrapeJob::GetIntSeconds(const string& str) const {
    int value = stoi(str.substr(0, str.size() - 1));
    if (str.back() == 's') {
        return value;
    } else if (str.back() == 'm') {
        return 60 * value;
    }
    return 30;
}

string ScrapeJob::ConvertMapParamsToQueryString() const {
    stringstream ss;
    for (auto it = mParams.begin(); it != mParams.end(); ++it) {
        const auto& key = it->first;
        const auto& values = it->second;
        for (const auto& value : values) {
            if (ss.tellp() != 0) {
                ss << "&";
            }
            ss << key << "=" << value;
        }
    }
    return ss.str();
}

bool ScrapeJob::BuildScrapeURL(Labels& labels, ScrapeTarget& st) const {
    string address = labels.Get("__address__");
    if (address.empty()) {
        return false;
    }

    // st.mScheme
    if (address.find("http://") == 0) {
        st.mScheme = "http";
        address = address.substr(strlen("http://"));
    } else if (address.find("https://") == 0) {
        st.mScheme = "https";
        address = address.substr(strlen("https://"));
    }

    // st.mMetricsPath
    auto n = address.find('/');
    if (n != string::npos) {
        st.mMetricsPath = address.substr(n);
        address = address.substr(0, n);
    }
    if (st.mMetricsPath.find('/') != 0) {
        st.mMetricsPath = "/" + st.mMetricsPath;
    }

    // st.mHost & st.mPort
    auto m = address.find(':');
    if (m != string::npos) {
        st.mHost = address.substr(0, m);
        st.mPort = stoi(address.substr(m + 1));
    } else {
        st.mHost = address;
        if (st.mScheme == "http") {
            st.mPort = 80;
        } else if (st.mScheme == "https") {
            st.mPort = 443;
        } else {
            return false;
        }
    }
    return true;
}

} // namespace logtail
