#ifndef ARGUS_COMMON_MODULE_DATA_H
#define ARGUS_COMMON_MODULE_DATA_H

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <boost/json.hpp>

namespace common {
    struct MetricData {
        std::map<std::string, double> valueMap;
        std::map<std::string, std::string> tagMap;

        bool check(int index = -1) const;
        std::string metricName() const;

        boost::json::object toJson() const;
    };
    struct CollectData {
        std::string moduleName;
        std::vector<MetricData> dataVector;

        boost::json::object toJson() const;
        void print(std::ostream &os = std::cout) const;
    };

#include "common/test_support"
class ModuleData {
public:
    static void convertCollectDataToString(const CollectData &collectData, std::string &content);
    static std::string convertCollectDataToString(const CollectData &collectData);
    static bool convertStringToCollectData(const std::string &content, CollectData &collectData, bool check = false);
    static std::string convert(const MetricData &metricData);
private:
    static void formatMetricData(const MetricData &metricData, std::stringstream &sStream);
    static bool getMetricData(std::stringstream &sStream, MetricData &metricData);
};
#include "common/test_support"

}
#endif  // !ARGUS_COMMON_MODULE_DATA_H
