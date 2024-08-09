# Native Delimiter Parsing Processor

## Introduction

The `processor_parse_delimiter_native` plugin parses event fields with delimiter formats and extracts new fields from the content within them.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| Type | string | Yes | `/` | Plugin type. Always set to `processor_parse_delimiter_native`. |
| SourceKey | string | Yes | `/` | The source field name. |
| Separator | string | Yes | `/` | The delimiter character. |
| Quote | string | No | `"` | The quote character used in the field. |
| Keys | [string] | Yes | `/` | List of fields to extract. |
| AllowingShortenedFields | bool | No | `true` | Whether to allow fewer extracted fields than the number in Keys. If not allowed, this will be treated as a parsing failure. |
| OverflowedFieldsTreatment | string | No | `extend` | Behavior when more fields are extracted than Keys. Options include:<ul><li>`extend`: Retains extra fields as separate log entries, with field names like \_\_column$i\_\_, where $i is the additional field number, starting from 0.</li><li>`keep`: Retains extra fields as a single entry, with field name \_\_column0\_\_.</li><li>`discard`: Discards extra fields.</li></ul> |
| KeepingSourceWhenParseFail | bool | No | `false` | Whether to keep the source field when parsing fails. |
| KeepingSourceWhenParseSucceed | bool | No | `false` | Whether to keep the source field when parsing succeeds. |
| RenamedSourceKey | string | No | `""` | Field name to use when keeping the source field. If not provided, the source field will not be renamed by default. |

## Example

Collect a delimiter-formatted file `/home/test-log/delimiter.log`, parse the log content, extract fields, and output the results to stdout.

- Input

```plain
127.0.0.1,07/Jul/2022:10:43:30 +0800,POST,PutData?Category=YunOsAccountOpLog,0.024,18204,200,37,-,aliyun-sdk-java
```

- Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/json.log
processors:
  - Type: processor_parse_delimiter_native
    SourceKey: content
    Separator: ','
    Quote: '"'
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
    "__tag__:__path__": "/home/test-log/json.log",
    "ip": "127.0.0.1",
    "time": "07/Jul/2022:10:43:30 +0800",
    "method": "POST",
    "url": "PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1657161028"
}
```
