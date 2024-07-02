# Native Processing Filter Regex

## Introduction

The `processor_filter_regex_native` plugin filters events based on the content of event fields.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| Type | string | Yes | / | Plugin type. Always set to `processor_filter_regex_native`. |
| FilterKey | [string] | Yes | / | Field names to filter, used in conjunction with `FilterRegex`. Events will be collected only if the specified key's field content meets the condition. Multiple conditions are "and" relationships, and the log will only be collected if all conditions are met. |
| FilterRegex | [string] | Yes | / | The corresponding regular expression for `FilterKey`. Must be the same length as `FilterKey`. |

## Example

Collect logs from the file `/home/test-log/reg.log`, parse the log content using a regular expression, extract fields, and only collect logs where the method is either POST or PUT and the status code is 200, outputting the results to stdout.

- Input

```plain
127.0.0.1 - - [07/Jul/2022:10:43:30 +0800] "POST /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"
127.0.0.1 - - [07/Jul/2022:10:44:30 +0800] "GET /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"
```

- Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/reg.log
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Regex: ([\d\.]+) \S+ \S+ \[(\S+) \S+\] \"(\w+) ([^\\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\\"]*)\" \"([^\\"]*)\"
    Keys:
      - ip
      - time
      - method
      - url
      - request_time
      - request_length
      - status
      - length
      - ref_url
      - browser
  - Type: processor_filter_regex_native
    FilterKey:
      - method
      - status
    FilterRegex:
      - ^(POST|PUT)$
      - ^200$
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

- Output

```json
{
    "__tag__:__path__": "/home/test-log/reg.log",
    "ip": "127.0.0.1",
    "time": "07/Jul/2022:10:43:30",
    "method": "POST",
    "url": "/PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1657161810"
}
```
