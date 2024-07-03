//
// Created by 韩呈杰 on 2023/4/19.
//
#include "common/CommonMetric.h"
#include "common/StringUtils.h"
#include "common/PrometheusMetric.h"

namespace common {
    CommonMetricBase::~CommonMetricBase() {
    }

    std::string CommonMetricBase::toString(int tabNum) const {
        std::stringstream ss;
        ss << StringUtils::GetTab(tabNum) << name << "{";
        const char *sep = "";

        typedef decltype(tagMap)::value_type CPair;
        std::vector<const CPair *> pairs;
        pairs.reserve(tagMap.size());
        for (auto const &it: tagMap) {
            pairs.push_back(&it);
        }
#ifdef ENABLE_COVERAGE
        // 单测时对key进行排序，以便结果是可预期的
        std::sort(pairs.begin(), pairs.end(), [](const CPair *l, const CPair *r) { return l->first < r->first; });
#endif
        for (auto const &it: pairs) {
            ss << sep;
            sep = ",";

            ss << StringUtils::GetTab(tabNum) << it->first << "=";
            ss << StringUtils::GetTab(tabNum) << '"' << _PromEsc(it->second) << '"';
        }
        ss << "} " << StringUtils::NumberToString(value);

        return ss.str();
    }

    std::string CommonMetric::toString(int tabNum) const {
        std::stringstream ss;
        ss << this->CommonMetricBase::toString(tabNum) << " " << timestamp;
        return ss.str();
    }
}
