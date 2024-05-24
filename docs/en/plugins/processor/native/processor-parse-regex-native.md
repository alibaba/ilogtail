# Regular Expression Native Processing Plugin

## Introduction

The `processor_parse_regex_native` plugin parses event fields using native regular expressions and extracts new fields directly from the content of the specified field.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| Type | string | Yes | `/` | Plugin type. Fixed as `processor_parse_regex_native`. |
| SourceKey | string | Yes | `/` | The name of the source field. |
| Regex | string | Yes | `/` | The regular expression to use for parsing. |
| Keys | [string] | Yes | `/` | List of fields to extract. |
| KeepingSourceWhenParseFail | bool | No | `false` | Whether to keep the source field when parsing fails. |
| KeepingSourceWhenParseSucceed | bool | No | `false` | Whether to keep the source field when parsing succeeds. |
| RenamedSourceKey | string | No | `""` | The new field name for the source field when it's kept. If not provided, the source field will not be renamed by default. |

## Example

Collect the file `/home/test-log/reg.log`, parse its log content using a regular expression, and extract fields, outputting the results to stdout.

- Input

```plain
127.0.0.1 - - [07/Jul/2022:10:43:30 +0800] "POST /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"
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
