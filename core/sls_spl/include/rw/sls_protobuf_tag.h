#pragma once

#include <stdint.h>
namespace apsara::sls::spl::protobuf_tag
{
#define SLS_PROTOBUF_TAG_GEN(index, type) (index << 3 | type)

    // pb encoding
    static const uint8_t varint = 0;
    static const uint8_t length_delimited = 2;
    static const uint8_t fixed_32 = 5;
    // sls_logs.proto
    // log
    static const uint8_t sls_logs_log_time = SLS_PROTOBUF_TAG_GEN(1, varint);
    static const uint8_t sls_logs_log_contents = SLS_PROTOBUF_TAG_GEN(2, length_delimited);
    static const uint8_t sls_logs_log_contents_key = SLS_PROTOBUF_TAG_GEN(1, length_delimited);
    static const uint8_t sls_logs_log_contents_value = SLS_PROTOBUF_TAG_GEN(2, length_delimited);
    static const uint8_t sls_logs_log_values = SLS_PROTOBUF_TAG_GEN(3, length_delimited); // currently not used
    static const uint8_t sls_logs_log_time_ns = SLS_PROTOBUF_TAG_GEN(4, fixed_32);

    // log_group
    static const uint8_t sls_logs_loggroup_logs = SLS_PROTOBUF_TAG_GEN(1, length_delimited);
    static const uint8_t sls_logs_loggroup_topic = SLS_PROTOBUF_TAG_GEN(3, length_delimited);
    static const uint8_t sls_logs_loggroup_source = SLS_PROTOBUF_TAG_GEN(4, length_delimited);
    static const uint8_t sls_logs_loggroup_tags = SLS_PROTOBUF_TAG_GEN(6, length_delimited);

    // tag
    static const uint8_t sls_logs_tag_key = SLS_PROTOBUF_TAG_GEN(1, length_delimited);
    static const uint8_t sls_logs_tag_value = SLS_PROTOBUF_TAG_GEN(2, length_delimited);

    // log_group list
    static const uint8_t sls_logs_log_group_list = SLS_PROTOBUF_TAG_GEN(1, length_delimited);
}