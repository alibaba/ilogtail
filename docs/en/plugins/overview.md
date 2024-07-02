# Overview

## Input

| Name | Provider | Introduction |
|-------------------------------------------------------------------------------|------------------------------------------------------------|-------------------------------------------------------|
| [`input_file`](input/input-file.md)<br> text log | SLS official | Text collection. |
| [`input_container_stdio`](input/input-container-stdio.md)<br> container standard output (native plug-in) | SLS official | Collect logs from container standard output/Standard error flow.                                                 |
| [`input_docker_stdout`](input/service-docker-stdout.md)<br> container standard output | SLS official | Collect logs from container standard output/Standard error flow.                                   |
| [`metric_debug_file`](input/metric-debug-file.md)<br> text log (debug) | SLS official | Plug-in used for debugging to read file content.                                       |
| [`metric_input_example`](input/metric-input-example.md)<br>MetricInput Example    | SLS official                                                      | MetricInput Example.                                      |
| [`metric_mock`](input/metric-mock.md)<br>Mock data-Metric | SLS official | Plug-in for generating metric simulation data.                                      |
| [`service_canal`](input/service-canal.md)<br>MySQL Binlog                     | SLS official                                                      | MySQL Binlog                             |
| [`service_http_server`](input/service-http-server.md)<br>Data from HTTP server               | SLS official                                                      | Revice data from unix socket、http/https、tcp.It also supports multiple protocols, such as sls protocol and otlp protocol. |
| [`service_input_example`](input/service-input-example.md)<br>ServiceInput Example | SLS official                                                      | ServiceInput Example.                                     |
| [`service_journal`](input/service-journal.md)<br>Journal data | SLS official | Collect Journal(systemd) logs of Linux system from the original binary file.               |
| [`service_kafka`](input/service-kafka.md)<br>Kafka                            | SLS official                                                      | Recive data from Kafka.                                  |
| [`service_mock`](input/service-mock.md)<br>Mock data-Service | SLS official | Plug-in for generating service analog data.                                     |
| [`service_otlp`](input/service-otlp.md)<br>OTLP data                             | Community<br>[`Zhu Shunjia`](https://github.com/shunjiazhu) | Receives OTLP data through http/grpc. |
| [`service_syslog`](input/service-syslog.md)<br>Syslog                       | SLS official                                                      | Recive syslog data.                                           |

## Process

### Native

| Name | Provider | Introduction |
| --- | ----- | ---- |
| [`processor_parse_regex_native`](processor/native/processor-parse-regex-native.md)<br> regular parsing native processing plug-in | SLS official | Use regular matching to parse events to specify fields and extract new fields. |
| [`processor_parse_json_native`](processor/native/processor-parse-json-native.md)<br>Json parsing native processing plug-in | SLS official | Parses the 'Json' format field content in an event and extracts new fields. |
| [`processor_parse_delimiter_native`](processor/native/processor-parse-delimiter-native.md)<br> delimiter parsing native processing plug-in | SLS official | Parses the delimiter format field content in an event and extracts new fields. |
| [`processor_parse_timestamp_native`](processor/native/processor-parse-timestamp-native.md)<br> time resolution native processing plug-in | SLS official | Parse the time field recorded in the event,And set the result to the__time__field of the event. |
| [`processor_filter_regex_native`](processor/native/processor-filter-regex-native.md)<br> filter native processing plug-in | SLS official | Filter events based on event fields. |
| [`processor_desensitize_native`](processor/native/processor-desensitize-native.md)<br> desensitization native processing plug-in | SLS official | Desensitizes the specified field content of the event. |

### Extension

| Name | Provider | Introduction |
| --- | ----- | ---- |
| [`processor_add_fields`](processor/extended/extenprocessor-add-fields.md)<br>add fields                          | SLS official                                                  | add fields.                            |
| [`processor_desensitize`](processor/extended/processor-desensitize.md)<br>Desensitize sensitive data                        | SLS official<br>[`Takuka0311`](https://github.com/Takuka0311) | Desensitize sensitive data. |
| [`processor_drop`](processor/extended/processor-drop.md)<br> drop field | SLS official | Drop field.                            |
| [`processor_fields_with_conditions`](processor/extended/processor-fields-with-condition.md)<br>Dynamically process based on conditions | Community<br>[`pj1987111`](https://github.com/pj1987111) | Dynamically expand or delete fields based on the values of some fields in the log. |
| [`processor_filter_regex`](processor/extended/processor-filter-regex.md)<br> log filtering | SLS official | Filters logs by regular match.                      |
| [`processor_gotime`](processor/extended/processor-gotime.md)<br>Gotime | SLS official | Parses the time field in the original log in the Go language time format.         |
| [`processor_grok`](processor/extended/processor-grok.md)<br>Grok                                      | SLS official<br>[`Takuka0311`](https://github.com/Takuka0311) | Use Grok syntax to process data |
| [`processor_json`](processor/extended/processor-json.md)<br>Json | SLS official | Implements parsing of logs in Json format.                  |
| [`processor_log_to_sls_metric`](processor/extended/processor-log-to-sls-metric.md)<br>logs to sls metric   | SLS official                                                  | trans logs to sls metric                   |
| [`processor_regex`](processor/extended/processor-regex.md)<br> regular | SLS official | The field extraction of text logs is realized through regular matching mode.            |
| [`processor_rename`](processor/extended/processor-rename.md)<br> rename field | SLS official | Rename field.                           |
| [`processor_split_char`](processor/extended/processor-delimiter.md)<br> delimiter | SLS official | Extract fields by single-character delimiter.                   |
| [`processor_split_string`](processor/extended/processor-delimiter.md)<br> delimiter | SLS official | Extract fields by multi-character delimiter.                   |
| [`processor_split_key_value`](processor/extended/processor-split-key-value.md)<br> key-value pair | SLS official | Extract fields by splitting key-value correctly.                  |
| [`processor_split_log_regex`](processor/extended/processor-split-log-regex.md)<br> multiline splitting | SLS official | Collects multiline logs (such as Java program logs).           |
| [`processor_string_replace`](processor/extended/processor-string-replace.md)<br>replace string                 | SLS official<br>[`pj1987111`](https://github.com/pj1987111) | Replace text logs by using full-text matching, regular matching, and escape characters. |

## Flusher

| Name | Provider | Introduction |
|------------------------------------------------------------------------------|-----------------------------------------------------|-------------------------------------------|
| [`flusher_kafka_v2`](flusher/flusher-kafka_v2.md)<br>Kafka                   | Community<br>[`shalousun`](https://github.com/shalousun) | Output the collected data to Kafka. |
| [`flusher_sls`](flusher/flusher-sls.md)<br>SLS | SLS official | Outputs the collected data to SLS.                            |
| [`flusher_stdout`](flusher/flusher-stdout.md)<br> standard output/file | SLS official | Outputs the collected data to the standard output or file.                        |
| [`flusher_otlp`](flusher/flusher-otlp.md)<br>OTLP flusher | Community<br>[`liuhaoyang`](https://GitHub.com/liuhaoyang) | The collected data supports the backend of 'Opentelemetry log protocol. |
| [`flusher_http`](flusher/flusher-http.md)<br>HTTP                            | Community<br>[`snakorse`](https://github.com/snakorse) | Outputs the collected data to the specified backend by http. |
| [`flusher_pulsar`](flusher/flusher-pulsar.md)<br>Kafka                       | Community<br>[`shalousun`](https://github.com/shalousun) | Output the collected data to Pulsar. |
| [`flusher_clickhouse`](flusher/flusher-clickhouse.md)<br>ClickHouse          | Community<br>[`kl7sn`](https://github.com/kl7sn) | Output the collected data to the ClickHouse. |
| [`flusher_elasticsearch`](flusher/flusher-elasticsearch.md)<br>ElasticSearch | Community<br>[`joeCarf`](https://github.com/joeCarf) | Output the collected data to the ElasticSearch. |
| [`flusher_loki`](flusher/flusher-loki.md)<br>Loki                                    | Community<br>[`abingcbc`](https://github.com/abingcbc) | Output the collected data to Loki. |
