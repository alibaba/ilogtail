#pragma once
#include <boost/regex.hpp>
#include <set>
#include <string>

#include "Labels.h"

namespace logtail {

using Action = std::string;

class RelabelConfig {
public:
    RelabelConfig();
    void validate();


    // A list of labels from which values are taken and concatenated
    // with the configured separator in order.
    std::vector<std::string> sourceLabels;
    // Separator is the string between concatenated values from the source labels.
    std::string separator;
    // Regex against which the concatenation is matched.
    boost::regex regex;
    // Modulus to take of the hash of concatenated values from the source labels.
    uint64_t modulus;
    // TargetLabel is the label to which the resulting string is written in a replacement.
    // Regexp interpolation is allowed for the replace action.
    std::string targetLabel;
    // Replacement is the regex replacement pattern to be used.
    std::string replacement;
    // Action is the action to be performed for the relabeling.
    Action action;

private:
};


namespace relabel {
    bool relabel(const RelabelConfig& cfg, LabelsBuilder& lb);
    bool processBuilder();
    bool process();
} // namespace relabel


} // namespace logtail
