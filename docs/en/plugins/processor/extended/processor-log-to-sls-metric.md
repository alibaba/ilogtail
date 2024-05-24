# Log Conversion to SLS Metric

## Introduction

The `processor_log_to_sls_metric` plugin allows you to configure logs to be transformed into SLS metrics based on provided settings.

## Stability

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter                 | Type     | Required | Description                                                                                             |
|---------------------------|--------|-------|-----------------------------------------------------------------------------------------------------------|
| MetricTimeKey             | String  | Optional | Specifies the field to use as the timestamp `__time_nano__`. Defaults to `log.Time`. Ensure the field is a valid timestamp, like seconds (10 digits), milliseconds (13 digits), microseconds (16 digits), or nanoseconds (19 digits). |
| MetricLabelKeys           | Array   | Required | A list of keys for the `__labels__` field, following the regex pattern: `^[a-zA-Z_][a-zA-Z0-9_]*$`. If the original log contains a `__labels__` field, it will be appended to this list. Label values cannot contain `\|` or `#$#`. |
| MetricValues              | Map     | Required | A mapping of time series field names (keys) to their corresponding value keys, following the regex pattern: `^[a-zA-Z_:][a-zA-Z0-9_:]*$`. Values must be double-type strings. |
| CustomMetricLabels        | Map     | Optional | Additional custom `__labels__` to be added. Keys must follow the same regex pattern as `MetricLabelKeys`. Values cannot contain `\|` or `#$#`. |
| IgnoreError               | Bool    | Optional | Whether to output an error log when a log does not match. Defaults to `false` if not specified.                |

**Note:** MetricTimeKey, MetricLabelKeys, MetricValues, and CustomMetricLabels fields must not overlap.

## Example

Here's a sample configuration demonstrating how to use the `processor_log_to_sls_metric` plugin:

### Input

```bash
echo '::1 - - [18/Jul/2022:07:28:01 +0000] "GET /hello/ilogtail HTTP/1.1" 404 153 "-" "curl/7.74.0" "-"' >> /home/test-log/nginx.log
```

### Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
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
  - Type: processor_log_to_sls_metric
    MetricLabelKeys:
      - url
      - method
    MetricValues:
      remote_addr: status
    CustomMetricLabels:
      nginx: test
    IgnoreError: false
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

### Output

```json
{
  "__labels__": "method#$#GET|nginx#$#test|url#$#/hello/ilogtail",
  "__name__": "::1",
  "__value__": "404",
  "__time_nano__": "1688956340000000000",
  "__time__": "1688956340"
}
```
