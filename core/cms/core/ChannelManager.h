#ifndef ARGUS_CORE_CHANNEL_MANAGER_H
#define ARGUS_CORE_CHANNEL_MANAGER_H

#include <string>
#include <vector>
#include <type_traits>
#include <memory>
#include <atomic>

#include "common/CommonMetric.h"
#include "common/SafeMap.h"

#include "OutputChannel.h"

namespace argus {
    // typedef enum {
    //     CONF_ERROR = 1,
    //     WRITE_FILE_ERROR = 2,
    //     WRITE_SLS_ERROR = 3,
    // } REPORT_STATUS_ERROR_CODE;

#include "common/test_support"
class ChannelManager {
public:
    typedef std::pair<std::string, std::string> ChanConf;

    ChannelManager();
    VIRTUAL ~ChannelManager();
    void Start();
    void End();
    std::shared_ptr<OutputChannel> GetOutputChannel(const std::string &name) const;

    template<typename T, typename std::enable_if<std::is_base_of<OutputChannel, T>::value, int>::type = 0>
    std::shared_ptr<T> Get(const std::string &name) const {
        return std::dynamic_pointer_cast<T>(GetOutputChannel(name));
    }

    bool GetOutputChannelStatus(const std::string &name, common::CommonMetric &metric) const;
    void AddOutputChannel(const std::string &name, std::shared_ptr<OutputChannel> pChannel);

    VIRTUAL int sendMsgToNextModule(const std::string &name, const std::vector<ChanConf> &outputVector,
                                    const std::chrono::system_clock::time_point &timestamp,
                                    int exitCode, const std::string &result, bool status,
                                    const std::string &mid = "") const;
    VIRTUAL int SendMetricToNextModule(const common::CommonMetric &metric,
                                       const std::vector<ChanConf> &outputVector) const;
    VIRTUAL int SendMetricsToNextModule(const std::vector<common::CommonMetric> &metrics,
                                        const std::vector<ChanConf> &outputVector) const;
    VIRTUAL int SendResultMapToNextModule(const std::vector<std::unordered_map<std::string, std::string>> &resultMaps,
                                          const std::vector<ChanConf> &outputVector) const;
    VIRTUAL int SendRawDataToNextModule(const common::RawData &rawData,
                                        const std::vector<ChanConf> &outputVector) const;
private:
    std::atomic<bool> mIsStart{false};
    SafeMap<std::string, std::shared_ptr<OutputChannel>> mChannelMap;
};
#include "common/test_support"

}
#endif
