# sls日志转换为sls metric

## 简介

`processor_sls_metric processor`插件可以根据配置将sls日志转换为sls metric。

## 版本

[Alpha](../stability-level.md)

## 配置参数

| 参数                 | 类型     | 必选或可选 | 参数说明                                                                                                                     |
|--------------------|--------|-------|--------------------------------------------------------------------------------------------------------------------------|
| MetricTimeKey      | String | 可选    | 指定要用作时间戳的字段。默认取`log.Time`。确保指定的字段是合法的、符合格式的纳秒时间戳(19位长）。                                                                  |
| MetricLabelKeys    | Array  | 可选    | labels字段的key列表，key需遵循正则表达式： `[a-zA-Z_][a-zA-Z0-9_]*`。如果原始字段中存在 `__labels__` 字段，该值将被追加到列表中。 Label的Value不能包含竖线（\|）和 "#$#"。 |
| MetricValues       | Map    | 可选    | 时序字段名所使用的key与时序值使用的key的映射。name需遵循正则表达式：`[a-zA-Z_:][a-zA-Z0-9_:]*` ，时序值需要是double类型的字符串。                                   |
| CustomMetricLabels | Map    | 可选    | 要追加的自定义标签。 key需遵循正则表达式： `[a-zA-Z_][a-zA-Z0-9_]*`，Value不能包含竖线（\|）和 "#$#"。                                                 |

注意：MetricTimeKey、MetricLabelKeys、MetricValues、CustomMetricLabels的字段不可相互重复。

## 样例

以下是一个示例配置，展示了如何使用 `processor_sls_metric` 插件来处理数据：

采集`/home/test-log/`路径下的`string_replace.log`文件，测试日志内容的正则匹配与替换功能。

* 输入

```bash
echo '::1 - - [18/Jul/2022:07:28:01 +0000] "GET /hello/ilogtail HTTP/1.1" 404 153 "-" "curl/7.74.0" "-"' >> /home/test-log/nginx.log
```

* 采集配置

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /logtail_host/host_mnt/Users/yitao/ilogtail-test/mock-log
    FilePattern: mock_nginx.log
    DockerFile: true

processors:
  - Type: processor_regex
    SourceKey: content
    Regex: '([\d\.:]+) - (\S+) \[(\S+) \S+\] \"(\S+) (\S+) ([^\\"]+)\" (\d+) (\d+) \"([^\\"]*)\" \"([^\\"]*)\" \"([^\\"]*)\"'
    Keys:
      - remote_addr
      - remote_user
      - time_local
      - method
      - url
      - protocol
      - status
      - body_bytes_sent
      - http_referer
      - http_user_agent
      - http_x_forwarded_for
  - Type: processor_sls_metric
    MetricLabelKeys:
      - url
      - method
    MetricValues:
      time_local: body_bytes_sent
      remote_addr: status
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```json
{
  "__labels__":"method#$#GET|url#$#/hello/ilogtail",
  "__name__":"::1",
  "__value__":"404",
  "__time_nano__":"1688729393000000000",
  "__time__":"1688729393"
}
```
```json
{
  "__labels__":"method#$#GET|url#$#/hello/ilogtail",
  "__name__":"18/Jul/2022:07:28:01",
  "__value__":"153",
  "__time_nano__":"1688729393000000000",
  "__time__":"1688729393"
}
```

在上述示例中，原始数据包含了一些标签和指标值。通过配置 `processor_sls_metric` 插件，我们指定了要使用的字段和标签，并生成了处理后的数据对象。处理后的数据对象包含了指定的标签、指标值和时间戳字段。

请根据您的实际需求和数据结构，调整配置和字段名称。