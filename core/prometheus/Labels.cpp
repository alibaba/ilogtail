#include "Labels.h"

#include <algorithm>

using namespace std;
namespace logtail {
Labels::Labels() {
}
std::string LabelsBuilder::get(std::string name) {
    for (auto label : add) {
        if (label.name == name) {
            return label.value;
        }
    }
    auto it = find(del.begin(), del.end(), [&name](Label l) { return l.name == name; });
    if (it != del.end()) {
        return "";
    }
    return base.get(name);
}
std::string Labels::get(std::string name) {
    for (auto label : labels) {
        if (label.name == name) {
            return label.value;
        }
    }
    return std::string();
}
} // namespace logtail
