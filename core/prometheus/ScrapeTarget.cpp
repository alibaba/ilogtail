#include "ScrapeTarget.h"

#include "Labels.h"

using namespace std;

namespace logtail {
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
} // namespace logtail