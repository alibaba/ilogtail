#pragma once

#include <cstdint>
#include <string>

namespace logtail {

namespace prometheus {
    const uint64_t PRIME64 = 1099511628211;
    const uint64_t OFFSET64 = 14695981039346656037ULL;
    const char* META_PREFIX = "__meta_";
    const std::string UNDEFINED = "undefined";
    const char* SOURCE_LABELS = "source_labels";
    const char* SEPARATOR = "separator";
    const char* TARGET_LABEL = "target_label";
    const char* REGEX = "regex";
    const char* REPLACEMENT = "replacement";
    const char* ACTION = "action";
    const char* MODULUS = "modulus";
} // namespace prometheus

} // namespace logtail