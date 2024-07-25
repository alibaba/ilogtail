#include "prometheus/ScrapeTarget.h"

#include "common/StringTools.h"
#include "prometheus/Constants.h"
#include "prometheus/Labels.h"

using namespace std;

namespace logtail {

ScrapeTarget::ScrapeTarget() : mPort(0) {
}
ScrapeTarget::ScrapeTarget(const Labels& labels) : mLabels(labels) {
    string address = mLabels.Get(prometheus::ADDRESS_LABEL_NAME);
    auto m = address.find(':');
    if (m != string::npos) {
        mHost = address.substr(0, m);
        mPort = stoi(address.substr(m + 1));
    }
}

string ScrapeTarget::GetHash() {
    return mHost + ":" + ToString(mPort) + ToString(mLabels.Hash());
}
} // namespace logtail