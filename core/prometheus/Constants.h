#pragma once

#include <string>

#include "Common.h"
using namespace std;
namespace ilogtail {

namespace prometheus {

    const string HTTP_SCHEME = "http";
    const string HTTPS_SCHEME = "https";
    const string JOB = "job";
    const string INSTANCE = "instance";
    const string ADDRESS = "address";
    const string __ADDRESS__ = "__address__";
    const string HTTP_PREFIX = "http://";
    const string HTTPS_PREFIX = "https://";
    const string JOB_NAME = "job_name";
    const string SCHEME = "scheme";
    const string METRICS_PATH = "metrics_path";
    const string SCRAPE_INTERVAL = "scrape_interval";
    const string SCRAPE_TIMEOUT = "scrape_timeout";
    const string AUTHORIZATION = "authorization";
    const string RELABEL_CONFIGS = "relabel_configs";
    const string __SCHEME__ = "__scheme__";
    const string __METRICS_PATH__ = "__metrics_path__";
    const string __SCRAPE_INTERVAL__ = "__scrape_interval__";
    const string __SCRAPE_TIMEOUT__ = "__scrape_timeout__";
    const string __PARAM_ = "__param_";
    const string LABELS = "labels";
    const string TARGETS = "targets";
    const string TYPE = "type";
    const string CREDENTIALS_FILE = "credentials_file";
    const string PARAMS = "params";

    const char* POD_NAME = "POD_NAME";
    const char* OPERATOR_PORT = "OPERATOR_PORT";
    const char* OPERATOR_HOST = "OPERATOR_HOST";

    const string ACCEPT_HEADER = "Accept";
    const string PROMETHEUS_REFRESH_HEADER = "X-Prometheus-Refresh-Interval-Seconds";
    const string USER_AGENT_HEADER = "User-Agent";


} // namespace prometheus

} // namespace ilogtail