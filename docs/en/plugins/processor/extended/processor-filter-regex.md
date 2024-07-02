# Log Filtering

## Introduction

The `processor_filter_regex processor` plugin allows for filtering of logs. A log will only be collected if it fully matches the regular expressions in the `Include` field and does not match any in the `Exclude` field. Logs that do not meet these criteria are discarded directly.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter                  | Type, Default | Description                                                                                     |
| -------------------------- | ------- | ------------------------------------------------------------------------------------------------- |
| Include                   | Map, `{}` | Key is a log field, Value is the regular expression that the field value should match. Key relationships are AND. A log is collected if all field values match their respective expressions. |
| Exclude                   | Map, `{}` | Key is a log field, Value is the regular expression that the field value should match. Key relationships are OR. A log is not collected if any field value matches its respective expression. |

## Example

Collect logs from the `/home/test-log/` directory, specifically the `proccessor-filter-regex.log` file, parse them in `Json` format, and filter some logs selectively.

* Input

```bash
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "aliyun-sdk-java"}' >> /home/test-log/proccessor-filter-regex.log
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "chrome"}' >> /home/test-log/proccessor-filter-regex.log
echo '{"ip": "192.168.**.**", "method": "POST", "brower": "aliyun-sls-ilogtail"}' >> /home/test-log/proccessor-filter-regex.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_json
    SourceKey: content
    KeepSource: false
    ExpandDepth: 1
    ExpandConnector: ""
  - Type: processor_filter_regex
    Include:
      ip: "10\\..*"
      method: POST
    Exclude:
      brower: "aliyun.*"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
  "__tag__:__path__": "/home/test-log/proccessor-filter-regex.log",
  "__time__": "1658837955",
  "brower": "chrome",
  "ip": "10.**.**.**",
  "method": "POST"
}
```
