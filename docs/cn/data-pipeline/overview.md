# 概览

## 输入

| 名称                                          | 提供方                                                       | 简介                                        |
|---------------------------------------------| ------------------------------------------------------------ |-------------------------------------------|
| `file_log`<br> 文本日志                         | SLS官方<br>[`messixukejia`](https://github.com/messixukejia) | 文本采集。                                     |
| `input_docker_stdout`<br>容器标准输出             | SLS官方                                                      | 从容器标准输出/标准错误流中采集日志。                       |
| `metric_debug_file`<br>文本日志（debug）          | SLS官方                                                      | 用于调试的读取文件内容的插件。                           |
| `metric_input_example`<br>MetricInput示例插件   | SLS官方                                                      | MetricInput示例插件。                          |
| `metric_meta_host`<br>主机Meta数据              | SLS官方                                                      | 主机Meta数据。                                 |
| `metric_mock`<br>Mock数据-Metric              | SLS官方                                                      | 生成metric模拟数据的插件。                          |
| `service_canal`<br>MySQL Binlog             | SLS官方                                                      | 将MySQL Binlog输入到iLogtail。                 |
| `service_input_example`<br>ServiceInput示例插件 | SLS官方                                                      | ServiceInput示例插件。                         |
| `service_journal`<br>Journal数据              | SLS官方                                                      | 从原始的二进制文件中采集Linux系统的Journal（systemd）日志。   |
| `service_kafka`<br>Kafka                    | SLS官方                                                      | 将Kafka数据输入到iLogtail。                      |
| `service_mock`<br>Mock数据-Service            | SLS官方                                                      | 生成service模拟数据的插件。                         |
| `service_mssql`<br>SqlServer查询数据            | SLS官方                                                      | 将Sql Server数据输入到iLogtail。                 |
| `service_pgsql`<br>PostgreSQL查询数据           | SLS官方                                                      | 将PostgresSQL数据输入到iLogtail。                |
| `service_syslog`<br>Syslog数据                | SLS官方                                                      | 采集syslog数据。                               |
| `service_gpu_metric`<br>GPU数据               | SLS官方                                                      | 支持收集英伟达GPU指标。                             |
| `observer_ilogtail_network`<br>无侵入网络调用数据    | SLS官方                                                      | 支持从网络系统调用中收集四层网络调用，并借助网络解析模块，可以观测七层网络调用细节。 |
| `service_http_server otlp`<br>HTTP OTLP数据 | SLS官方 | 通过http协议，接收OTLP数据。 |

## 处理

| 名称                                               | 提供方                                              | 简介                                             |
| -------------------------------------------------- | --------------------------------------------------- | ------------------------------------------------ |
| `processor_add_fields`<br>添加字段                 | SLS官方                                             | 添加字段。                                       |
| `processor_default`<br>原始数据                    | SLS官方                                             | 不对数据任何操作，只是简单的数据透传。           |
| `processor_desensitize`<br>数据脱敏                    | SLS官方<br>[`Takuka0311`](https://github.com/Takuka0311) | 对敏感数据进行脱敏处理。           |
| `processor_drop`<br>丢弃字段                       | SLS官方                                             | 丢弃字段。                                       |
| `processor_encrypt`<br>字段加密                   | SLS官方                                               | 加密字段                                  |
| `processor_fields_with_conditions`<br>条件字段处理 | 社区<br>[`pj1987111`](https://github.com/pj1987111) | 根据日志部分字段的取值，动态进行字段扩展或删除。 |
| `processor_filter_regex`<br>日志过滤               | SLS官方                                             | 通过正则匹配过滤日志。                           |
| `processor_grok`<br>Grok                          | SLS官方<br>[`Takuka0311`](https://github.com/Takuka0311) | 通过 Grok 语法对数据进行处理              |
| `processor_json`<br>Json                           | SLS官方                                             | 实现对Json格式日志的解析。                       |
| `processor_regex`<br>正则                          | SLS官方                                             | 通过正则匹配的模式实现文本日志的字段提取。       |
| `processor_rename`<br>重命名字段                   | SLS官方                                             | 重命名字段。                                     |
| `processor_split_char`<br>分隔符                   | SLS官方                                             | 通过单字符的分隔符提取字段。                     |
| `processor_split_key_value`<br>键值对              | SLS官方                                             | 通过切分键值对的方式提取字段。                   |
| `processor_split_log_regex`<br>多行切分            | SLS官方                                             | 实现多行日志（例如Java程序日志）的采集。         |
| `processor_split_string`<br>分隔符                 | SLS官方                                             | 通过多字符的分隔符提取字段。                     |

## 聚合

| 名称                               | 提供方                                                 | 简介                                        |
|----------------------------------|-----------------------------------------------------|---------------------------------------------|
| `aggregator_content_value_group` | 社区<br>[`snakorse`](https://github.com/snakorse)     | 按照指定的Key对采集到的数据进行分组聚合           |
| `aggregator_metadata_group`      | 社区<br>[`urnotsally`](https://github.com/urnotsally) | 按照指定的Metadata Keys对采集到的数据进行重新分组聚合|

## 输出

| 名称                                       | 提供方                                                 | 简介                                        |
|------------------------------------------|-----------------------------------------------------|-------------------------------------------|
| `flusher_kafka`<br>Kafka                 | 社区                                                  | 将采集到的数据输出到Kafka。推荐使用下面的flusher_kafka_v2   |
| `flusher_kafka_v2`<br>Kafka              | 社区<br>[`shalousun`](https://github.com/shalousun)   | 将采集到的数据输出到Kafka。                          |
| `flusher_sls`<br>SLS                     | SLS官方                                               | 将采集到的数据输出到SLS。                            |
| `flusher_stdout`<br>标准输出/文件              | SLS官方                                               | 将采集到的数据输出到标准输出或文件。                        |
| `flusher_otlp_log`<br>OTLP日志             | 社区<br>[`liuhaoyang`](https://github.com/liuhaoyang) | 将采集到的数据支持`Opentelemetry log protocol`的后端。 |
| `flusher_http`<br>HTTP                   | 社区<br>[`snakorse`](https://github.com/snakorse)     | 将采集到的数据以http方式输出到指定的后端。                   |
| `flusher_pulsar`<br>Kafka                | 社区<br>[`shalousun`](https://github.com/shalousun)   | 将采集到的数据输出到Pulsar。                         |
| `flusher_clickhouse`<br>ClickHouse       | 社区<br>[`kl7sn`](https://github.com/kl7sn)           | 将采集到的数据输出到ClickHouse。                     |
| `flusher_elasticsearch`<br>ElasticSearch | 社区<br>[`joeCarf`](https://github.com/joeCarf)       | 将采集到的数据输出到ElasticSearch。                  |

## 加速

| 名称                                           | 提供方  | 简介                                           |
| ---------------------------------------------- | ------- | ---------------------------------------------- |
| `processor_delimiter_accelerate`<br>分隔符加速 | SLS官方 | 以加速模式实现分隔符日志的字段提取。           |
| `processor_json_accelerate`<br>Json加速        | SLS官方 | 以加速模式实现`Json`格式日志的字段提取。       |
| `processor_regex_accelerate`<br>正则加速       | SLS官方 | 通过正则匹配以加速模式实现文本日志的字段提取。 |


## 扩展

### ClientAuthenticator

| 名称                          | 提供方                                             | 简介                             |
|-----------------------------|-------------------------------------------------|--------------------------------|
| `ext_basicauth`<br> Basic认证 | 社区<br>[`snakorse`](https://github.com/snakorse) | 为 http_flusher 插件提供 basic 认证能力 |

### FlushInterceptor

| 名称                                     | 提供方                                             | 简介                                        |
|----------------------------------------|-------------------------------------------------|-------------------------------------------|
| `ext_groupinfo_filter`<br> GroupInfo过滤 | 社区<br>[`snakorse`](https://github.com/snakorse) | 为 http_flusher 插件提供根据GroupInfo筛选最终提交数据的能力 |

### Decoder

| 协议(Format)       | 提供方                                                 | 简介                                            |
|------------------|-----------------------------------------------------|-----------------------------------------------|
| `sls`            | SLS官方                                               | 解析sls日志                                       |
| `prometheus`     | SLS官方                                               | 解析 prometheus expformat 和 remote-storage 协议数据 |
| `influx`         | SLS官方                                               | 解析 influx 协议数据                                |
| `influxdb`       | SLS官方                                               | 解析 influxdb 协议数据                              |
| `statsd`         | SLS官方                                               | 解析 statsd 协议数据                                |
| `otlp_logv1`     | SLS官方                                               | 解析 opentelemetry log 协议数据                     |
| `otlp_metricsv1` | 社区<br>[`shunjiazhu`](https://github.com/shunjiazhu) | 解析 opentelemetry metrics 协议数据                 |
| `otlp_tracev1`   | 社区<br>[`shunjiazhu`](https://github.com/shunjiazhu) | 解析 opentelemetry trace 协议数据                   |
| `raw`            | 社区<br>[`urnotsally`](https://github.com/urnotsally) | 数据流直接封装为ByteArrayEvent                        |
| `pyroscope`      | SLS官方                                               | 解析 pyroscope 数据                               |
