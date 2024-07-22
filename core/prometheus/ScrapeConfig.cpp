
#include "ScrapeConfig.h"

#include <json/value.h>

#include <string>

#include "common/FileSystemUtil.h"
#include "common/StringTools.h"
#include "logger/Logger.h"

using namespace std;

namespace logtail {
ScrapeConfig::ScrapeConfig()
    : mScheme("http"),
      mMetricsPath("/metrics"),
      mScrapeInterval(60),
      mScrapeTimeout(10),
      mMaxScrapeSize(-1),
      mSampleLimit(-1),
      mSeriesLimit(-1) {
}
bool ScrapeConfig::Init(const Json::Value& scrapeConfig) {
    if (scrapeConfig.isMember("job_name") && scrapeConfig["job_name"].isString()) {
        mJobName = scrapeConfig["job_name"].asString();
        if (mJobName.empty()) {
            return false;
        }
    } else {
        return false;
    }
    if (scrapeConfig.isMember("scheme") && scrapeConfig["scheme"].isString()) {
        mScheme = scrapeConfig["scheme"].asString();
    }
    if (scrapeConfig.isMember("metrics_path") && scrapeConfig["metrics_path"].isString()) {
        mMetricsPath = scrapeConfig["metrics_path"].asString();
    }
    if (scrapeConfig.isMember("scrape_interval") && scrapeConfig["scrape_interval"].isString()) {
        string tmpScrapeIntervalString = scrapeConfig["scrape_interval"].asString();
        if (EndWith(tmpScrapeIntervalString, "s")) {
            mScrapeInterval = stoll(tmpScrapeIntervalString.substr(0, tmpScrapeIntervalString.find("s")));
        } else if (EndWith(tmpScrapeIntervalString, "m")) {
            mScrapeInterval = stoll(tmpScrapeIntervalString.substr(0, tmpScrapeIntervalString.find("m"))) * 60;
        }
    }
    if (scrapeConfig.isMember("scrape_timeout") && scrapeConfig["scrape_timeout"].isString()) {
        string tmpScrapeTimeoutString = scrapeConfig["scrape_timeout"].asString();
        if (EndWith(tmpScrapeTimeoutString, "s")) {
            mScrapeTimeout = stoll(tmpScrapeTimeoutString.substr(0, tmpScrapeTimeoutString.find("s")));
        } else if (EndWith(tmpScrapeTimeoutString, "m")) {
            mScrapeTimeout = stoll(tmpScrapeTimeoutString.substr(0, tmpScrapeTimeoutString.find("m"))) * 60;
        }
    }
    // <size>: a size in bytes, e.g. 512MB. A unit is required. Supported units: B, KB, MB, GB, TB, PB, EB.
    if (scrapeConfig.isMember("max_scrape_size") && scrapeConfig["max_scrape_size"].isString()) {
        string tmpMaxScrapeSize = scrapeConfig["max_scrape_size"].asString();
        if (tmpMaxScrapeSize.empty()) {
            mMaxScrapeSize = -1;
        } else if (EndWith(tmpMaxScrapeSize, "KiB") || EndWith(tmpMaxScrapeSize, "K")
                   || EndWith(tmpMaxScrapeSize, "KB")) {
            tmpMaxScrapeSize = tmpMaxScrapeSize.substr(0, tmpMaxScrapeSize.find("K"));
            mMaxScrapeSize = stoll(tmpMaxScrapeSize) * 1024;
        } else if (EndWith(tmpMaxScrapeSize, "MiB") || EndWith(tmpMaxScrapeSize, "M")
                   || EndWith(tmpMaxScrapeSize, "MB")) {
            tmpMaxScrapeSize = tmpMaxScrapeSize.substr(0, tmpMaxScrapeSize.find("M"));
            mMaxScrapeSize = stoll(tmpMaxScrapeSize) * 1024 * 1024;
        } else if (EndWith(tmpMaxScrapeSize, "GiB") || EndWith(tmpMaxScrapeSize, "G")
                   || EndWith(tmpMaxScrapeSize, "GB")) {
            tmpMaxScrapeSize = tmpMaxScrapeSize.substr(0, tmpMaxScrapeSize.find("G"));
            mMaxScrapeSize = stoll(tmpMaxScrapeSize) * 1024 * 1024 * 1024;
        } else if (EndWith(tmpMaxScrapeSize, "TiB") || EndWith(tmpMaxScrapeSize, "T")
                   || EndWith(tmpMaxScrapeSize, "TB")) {
            tmpMaxScrapeSize = tmpMaxScrapeSize.substr(0, tmpMaxScrapeSize.find("T"));
            mMaxScrapeSize = stoll(tmpMaxScrapeSize) * 1024 * 1024 * 1024 * 1024;
        } else if (EndWith(tmpMaxScrapeSize, "PiB") || EndWith(tmpMaxScrapeSize, "P")
                   || EndWith(tmpMaxScrapeSize, "PB")) {
            tmpMaxScrapeSize = tmpMaxScrapeSize.substr(0, tmpMaxScrapeSize.find("P"));
            mMaxScrapeSize = stoll(tmpMaxScrapeSize) * 1024 * 1024 * 1024 * 1024 * 1024;
        } else if (EndWith(tmpMaxScrapeSize, "EiB") || EndWith(tmpMaxScrapeSize, "E")
                   || EndWith(tmpMaxScrapeSize, "EB")) {
            tmpMaxScrapeSize = tmpMaxScrapeSize.substr(0, tmpMaxScrapeSize.find("E"));
            mMaxScrapeSize = stoll(tmpMaxScrapeSize) * 1024 * 1024 * 1024 * 1024 * 1024 * 1024;
        } else if (EndWith(tmpMaxScrapeSize, "B")) {
            tmpMaxScrapeSize = tmpMaxScrapeSize.substr(0, tmpMaxScrapeSize.find("B"));
            mMaxScrapeSize = stoll(tmpMaxScrapeSize);
        }
    }

    if (scrapeConfig.isMember("sample_limit") && scrapeConfig["sample_limit"].isInt64()) {
        mSampleLimit = scrapeConfig["sample_limit"].asInt64();
    }
    if (scrapeConfig.isMember("series_limit") && scrapeConfig["series_limit"].isInt64()) {
        mSeriesLimit = scrapeConfig["series_limit"].asInt64();
    }
    if (scrapeConfig.isMember("params") && scrapeConfig["params"].isObject()) {
        const Json::Value& params = scrapeConfig["params"];
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

    if (scrapeConfig.isMember("authorization") && scrapeConfig["authorization"].isObject()) {
        string type = scrapeConfig["authorization"]["type"].asString();
        string bearerToken;
        bool b = ReadFile(scrapeConfig["authorization"]["credentials_file"].asString(), bearerToken);
        if (!b) {
            LOG_ERROR(sLogger,
                      ("read credentials_file failed, credentials_file",
                       scrapeConfig["authorization"]["credentials_file"].asString()));
        }
        mHeaders["Authorization"] = type + " " + bearerToken;
        LOG_INFO(
            sLogger,
            ("read credentials_file success, credentials_file",
             scrapeConfig["authorization"]["credentials_file"].asString())("Authorization", mHeaders["Authorization"]));
    }

    for (const auto& relabelConfig : scrapeConfig["relabel_configs"]) {
        mRelabelConfigs.emplace_back(relabelConfig);
    }

    return true;
}
} // namespace logtail