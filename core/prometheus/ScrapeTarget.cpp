#include "ScrapeTarget.h"

#include "Labels.h"
#include "StringTools.h"

using namespace std;

namespace logtail {

ScrapeTarget::ScrapeTarget() {
}
ScrapeTarget::ScrapeTarget(const Labels& labels) {
    mLabels = labels;
    // host & port
    string address = mLabels.Get("__address__");
    auto m = address.find(':');
    if (m != string::npos) {
        mHost = address.substr(0, m);
        mPort = stoi(address.substr(m + 1));
    }
}

ScrapeTarget::ScrapeTarget(const ScrapeTarget& other) {
    mHost = other.mHost;
    mLabels = other.mLabels;
    mPort = other.mPort;
}

ScrapeTarget& ScrapeTarget::operator=(const ScrapeTarget& other) {
    mHost = other.mHost;
    mLabels = other.mLabels;
    mPort = other.mPort;
    return *this;
}

string ScrapeTarget::GetHash() {
    return mHost + ":" + ToString(mPort) + ToString(mLabels.Hash());
}
} // namespace logtail