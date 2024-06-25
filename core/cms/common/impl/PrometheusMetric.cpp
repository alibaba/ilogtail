#include "common/PrometheusMetric.h"

#include <cmath>
#include <cctype>    // isspace, isalpha, isalnum
#include <boost/algorithm/string.hpp> // is_any_of
#include <boost/range/algorithm/replace_copy_if.hpp>  // replace_copy_if

#include "common/StringUtils.h"
#include "common/Logger.h"
#include "common/ArgusMacros.h"

using namespace std;

namespace common {
    extern const std::map<std::string, double> &mapSpecialDouble;

    int PrometheusMetric::ParseMetrics(const std::string &metricsStr, std::vector<CommonMetric> &metrics) {
        // string str = metricsStr;
        std::vector<std::string> lines = StringUtils::split(metricsStr, '\n', false);
        metrics.clear();
        for (const auto &line: lines) {
            CommonMetric commonMetric;
            if (ParseMetric(line, commonMetric) == 0) {
                metrics.push_back(commonMetric);
            }
        }
        return static_cast<int>(metrics.size());
    }

    // 返回值：0:成功，-1: 未知错误, > 0: 出错的位置
    int PrometheusMetric::ParseMetric(const string &lineStr, CommonMetric &commonMetric) {
        string line = TrimSpace(lineStr);
        // 跳过#号开头的『注释』行
        if (line.empty() || line[0] == '#') {
            return -1;
        }

        int ret = 0;
        // See reference:
        // 1. https://prometheus.io/docs/concepts/data_model/ # 《Prometheus Data Model》
        // 2. https://prometheus.io/docs/instrumenting/exposition_formats/ # 《EXPOSITION FORMATS》
        auto throwException = [&](const char *s, const std::string &errMsg) {
            ret = static_cast<int>(s - line.c_str() + 1);

            std::stringstream ss;
            std::string escapedLine;
            boost::replace_copy_if(line, std::back_inserter(escapedLine), boost::is_any_of("\r\n\t"), ' ');
            ss << escapedLine << std::endl;
            if (s > line.c_str()) {
                ss << std::string(static_cast<size_t>(s - line.c_str()), '.');
            }
            ss << "^" << std::endl;
            ss << errMsg;
            throw std::runtime_error(ss.str());
        };

        const char *input = line.c_str();
        auto next = [&]() {
            for (; *input && std::isspace(*(const unsigned char *) input); input++) {
            }
            return input;
        };
        auto parseMetricName = [&]() {
            // 指标名：[a-zA-Z_:][a-zA-Z0-9_:]*
            next();
            // 指标名首字母
            const char *start = input;
            if (*start != '{') {
                if (*start != ':' && *start != '_' && !std::isalpha(*(const unsigned char *) start)) {
                    throwException(start, "the first char of metric name don't satisfied: [a-zA-Z_:]");
                }
                for (input++; *input; ++input) {
                    if (!(*input == ':' || *input == '_' || std::isalnum(*(const unsigned char *) input))) {
                        break;
                    }
                }
                // metric name结束后，须跟label或value，否则判定为『非法』
                if (*input != '{' && !std::isspace(*(const unsigned char *) input)) {
                    throwException(input, fmt::format(
                            "invalid metricName char: '{}', validated regex: [a-zA-Z_:][a-zA-Z0-9_:]*", *input));
                }
            }
            return std::string{start, input};
        };
        auto parseLabelName = [&](size_t count) {
            next(); // skip space
            const char *start = input;
            if (*start != '_' && !std::isalpha(*(const unsigned char *) start)) {
                throwException(start, "the first char of label name don't satisfied: [a-zA-Z_]");
            }
            for (input++; *input; ++input) {
                if (*input != '_' && !std::isalnum(*(const unsigned char *) input)) {
                    break;
                }
            }
            return std::string{start, input};
        };
        auto parseLabelValue = [&]() {
            next();
            if (*input != '"') {
                throwException(input, "format error: label value should start with '\"'");
            }
            input++;
            std::string v;
            for (; *input && *input != '"'; ++input) {
                if (*input == '\\') {
                    char nc = input[1];
                    if (nc == 'n') {
                        ++input;
                        v.push_back('\n');
                        continue;
                    } else if (nc == '\\' || nc == '"') {
                        ++input;
                    }
                }
                v.push_back(*input);
            }
            if (*input == '"') {
                input++;
            }
            // 按照Prometheus文档，此处不应当trim, 但以前处理了。
#ifdef WIN32
            v = Trim(v, " \t\r");
#else
            v = Trim(v, " \t");
#endif

            return v;
        };
        auto parseLabels = [&]() {
            std::map<std::string, std::string> labels;

            next();
            if (*input == '{') {
                ++input;
                for (const char *c = next(); *c && *c != '}'; c = next()) {
                    std::string labelName = parseLabelName(labels.size());
                    next();
                    if (*input != '=') {
                        throwException(input, "format error: expected '='");
                    }
                    input++;
                    std::string labelValue = parseLabelValue();
                    labels[labelName] = labelValue;

                    next();
                    if (*input == ',') {
                        input++;
                    } else if (*input != '}') {
                        throwException(input, "format error: expected '}'");
                    }
                }
                if (*input == '}') {
                    input++;
                }
            }
            RETURN_RVALUE(labels);
        };
        auto parseValue = [&]() {
            next();
            if (!*input) {
                throwException(input, "unexpected EOF");
            }
            const char *start = input;
            for (; *input && !std::isspace(*(const unsigned char *) input); ++input) {
            }

            double v = 0;
            std::string s{start, input};
            auto it = mapSpecialDouble.find(ToLower(s));
            if (it != mapSpecialDouble.end()) {
                v = it->second;
            } else {
                convert(s, v);
            }
            return v;
        };
        auto parseTimestamp = [&]() {
            next();
            int64_t ts = 0;
            if (*input) {
                convert(input, ts);
            }
            return ts;
        };

        try {
            commonMetric.name = parseMetricName();
            commonMetric.tagMap = parseLabels();
            commonMetric.value = parseValue();
            commonMetric.timestamp = parseTimestamp();
        } catch (const std::exception &ex) {
            LogDebug("\n{}", ex.what());
            ret = (ret == 0 ? -1 : ret);
        }
        return ret;
    }

    string PrometheusMetric::MetricToLine(const CommonMetric &commonMetric, bool needTimestamp) {
        // {} 1.01
        // cpu_total 1.01
        // cpu_total{instanceId="i-xxx"} 1.01
        string line = commonMetric.name;
        if (commonMetric.name.empty() || !commonMetric.tagMap.empty()) {
            line += "{";
            const char *sep = "";
            for (const auto &it: commonMetric.tagMap) {
                line.append(sep).append(it.first).append("=\"").append(_PromEsc(it.second)).append("\"");
                sep = ",";
            }
            line += "}";
        }

        // https://prometheus.io/docs/instrumenting/exposition_formats/#comments-help-text-and-type-information
        line.append(" ");
        if (std::isnan(commonMetric.value)) {
            line.append("NaN");
        } else if (std::isinf(commonMetric.value)) {
            line.append(commonMetric.value > 0 ? "+Inf" : "-Inf");
        } else {
            line.append(fmt::format("{}", commonMetric.value));
        }

        if (needTimestamp) {
            line.append(" ").append(convert(commonMetric.timestamp));
        }

        return line;
    }

    template<typename T>
    static void handleEscape(const std::string &src, T &dst) {
        for (auto ch: src) {
            if (ch == '\\' || ch == '\"') {
                dst.push_back('\\');
                dst.push_back(ch);
            } else if (ch == '\n') {
                dst.push_back('\\');
                dst.push_back('n');
            } else {
                dst.push_back(ch);
            }
        }
    }

    string PrometheusMetric::HandleEscape(const std::string &valueStr) {
        struct tagCounter {
            size_t count = 0;
            void push_back(char) {
                count++;
            }
        } counter;
        handleEscape(valueStr, counter);
        if (counter.count == valueStr.size()) {
            return valueStr;
        }

        string newStr;
        newStr.reserve(counter.count);
        handleEscape(valueStr, newStr);

        return newStr;
    }
}