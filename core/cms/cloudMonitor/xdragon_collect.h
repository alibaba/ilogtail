#ifndef ARGUS_XDRAGON_COLLECT_H
#define ARGUS_XDRAGON_COLLECT_H

#include <string>

namespace cloudMonitor {

#include "common/test_support"
class XDragonCollect {
public:
    XDragonCollect();
    ~XDragonCollect();

    int Init();

    int Collect(std::string &data);

private:
    bool CheckEvn();
    bool GetCollectResult(std::string &result) const;

private:
    std::string mModuleName;
    std::string mBinFile;
    std::string mIniFile;
    size_t mCount = 0;
    size_t mTotalCount = 0;
};
#include "common/test_support"

}
#endif
