#include "MetricConstants.h"

namespace logtail {


const std::string METRIC_FIELD_REGION = "region";
const std::string METRIC_REGION_DEFAULT = "default";
const std::string METRIC_SLS_LOGSTORE_NAME = "shennong_log_profile";
const std::string METRIC_TOPIC_TYPE = "logtail_metric";
const std::string METRIC_TOPIC_FIELD_NAME = "__topic__";

const std::string LABEL_PREFIX = "label.";
const std::string VALUE_PREFIX = "value.";


// processor common metrics
const std::string METRIC_PROC_IN_RECORDS_TOTAL = "proc_in_records_total";
const std::string METRIC_PROC_IN_RECORDS_SIZE_BYTES = "proc_in_records_size_bytes";
const std::string METRIC_PROC_OUT_RECORDS_TOTAL = "proc_out_records_total";
const std::string METRIC_PROC_OUT_RECORDS_SIZE_BYTES = "proc_out_records_size_bytes";
const std::string METRIC_PROC_DISCARD_RECORDS_TOTAL = "proc_discard_records_total";
const std::string METRIC_PROC_TIME_MS = "proc_time_ms";

// processor cunstom metrics
const std::string METRIC_PROC_PARSE_IN_SIZE_BYTES = "proc_parse_in_size_bytes";
const std::string METRIC_PROC_PARSE_OUT_SIZE_BYTES = "proc_parse_out_size_bytes";

const std::string METRIC_PROC_PARSE_ERROR_TOTAL = "proc_parse_error_total";
const std::string METRIC_PROC_KEY_COUNT_NOT_MATCH_ERROR_TOTAL = "proc_key_count_not_match_error_total";
const std::string METRIC_PROC_HISTORY_FAILURE_TOTAL = "proc_history_failure_total";

// processor filter metrics
const std::string METRIC_PROC_FILTER_IN_SIZE_BYTES = "proc_filter_in_size_bytes";
const std::string METRIC_PROC_FILTER_OUT_SIZE_BYTES = "proc_filter_out_size_bytes";
const std::string METRIC_PROC_FILTER_ERROR_TOTAL = "proc_filter_error_total";
const std::string METRIC_PROC_FILTER_RECORDS_TOTAL = "proc_filter_records_total";


// processore plugin name
const std::string PLUGIN_PROCESSOR_PARSE_REGEX_NATIVE = "processor_parse_regex_native";

// processor desensitize metrics
const std::string METRIC_PROC_DESENSITIZE_RECORDS_TOTAL = "proc_desensitize_records_total";

}