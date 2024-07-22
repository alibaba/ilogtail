#pragma once

#include <string>

namespace ilogtail {

namespace prometheus {

    const std::string HTTP_SCHEME = "http";
    const std::string HTTPS_SCHEME = "https";
    const std::string JOB = "job";
    const std::string INSTANCE = "instance";
    const std::string ADDRESS = "address";
    const std::string __ADDRESS__ = "__address__";
    const std::string HTTP_PREFIX = "http://";
    const std::string HTTPS_PREFIX = "https://";
    const std::string JOB_NAME = "job_name";
    const std::string SCHEME = "scheme";
    const std::string METRICS_PATH = "metrics_path";
    const std::string SCRAPE_INTERVAL = "scrape_interval";
    const std::string SCRAPE_TIMEOUT = "scrape_timeout";
    const std::string AUTHORIZATION = "authorization";
    const std::string RELABEL_CONFIGS = "relabel_configs";
    const std::string __SCHEME__ = "__scheme__";
    const std::string __METRICS_PATH__ = "__metrics_path__";
    const std::string __SCRAPE_INTERVAL__ = "__scrape_interval__";
    const std::string __SCRAPE_TIMEOUT__ = "__scrape_timeout__";
    const std::string __PARAM_ = "__param_";
    const std::string LABELS = "labels";
    const std::string TARGETS = "targets";
    const std::string TYPE = "type";
    const std::string CREDENTIALS_FILE = "credentials_file";
    const std::string PARAMS = "params";

    const char* POD_NAME = "POD_NAME";
    const char* OPERATOR_PORT = "OPERATOR_PORT";
    const char* OPERATOR_HOST = "OPERATOR_HOST";

    const std::string ACCEPT_HEADER = "Accept";
    const std::string PROMETHEUS_REFRESH_HEADER = "X-Prometheus-Refresh-Interval-Seconds";
    const std::string USER_AGENT_HEADER = "User-Agent";


} // namespace prometheus

} // namespace ilogtail