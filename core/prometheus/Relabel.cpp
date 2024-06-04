#include "Relabel.h"

using namespace std;
namespace logtail {
RelabelConfig::RelabelConfig() {
}

bool relabel::relabel(const RelabelConfig& cfg, LabelsBuilder& lb) {
    vector<string> values;
    for (auto item : cfg.sourceLabels) {
        values.push_back(lb.get(item));
    }
    // ...
    return false;
}
} // namespace logtail
