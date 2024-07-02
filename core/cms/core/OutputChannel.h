#ifndef _OUTPUT_CHANNEL_H_
#define _OUTPUT_CHANNEL_H_
#include <string>
#include <vector>
#include <unordered_map>
#include "common/CommonMetric.h"

#include "common/ThreadWorker.h"

namespace argus
{
class OutputChannel: public common::ThreadWorker {
private:
    virtual void doRun() override {}
public:
    OutputChannel() = default;
    ~OutputChannel() override = default;
    // 原始timestamp为int64, 单位为: 秒
    virtual int addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                            int exitCode, const std::string &result, const std::string &conf, bool reportStatus,
                            const std::string &mid) = 0;

    virtual int AddCommonMetric(const common::CommonMetric &metric,std::string conf){
		return 0;
	}
    virtual int AddCommonMetrics(const std::vector<common::CommonMetric> &metrics,std::string conf){
		return 0;
	}
    virtual int AddResultMap(const std::vector<std::unordered_map<std::string, std::string>> &resultMaps,
                              std::string conf) {
		return 0;
	}
    virtual int AddRawData(const common::RawData &rawData,std::string conf){
		return 0;
	}
    virtual void GetChannelStatus(common::CommonMetric &metric){}

    static std::string ToPayloadString(double value);
};
}
#endif
