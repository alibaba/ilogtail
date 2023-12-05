#include "spl/SplConstants.h"

namespace apsara::sls::spl {
    const std::string FIELD_TIMESTAMP = "__timestamp__";
    const std::string FIELD_TIMESTAMP_NANOSECOND = "__timestampNanosecond__";
    const std::string FIELD_PREFIX_TAG = "__tag__:";
    const std::string FIELD_CONTENT = "content";


    const size_t LENGTH_FIELD_PREFIX_TAG = FIELD_PREFIX_TAG.length();
    const size_t LENGTH_FIELD_TIMESTAMP = FIELD_TIMESTAMP.length();
    const size_t LENGTH_FIELD_TIMESTAMP_NANOSECOND = FIELD_TIMESTAMP_NANOSECOND.length();

    const std::string NULL_STR = "null";

} // namespace apsara::sls::spl