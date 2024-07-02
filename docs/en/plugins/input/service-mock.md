# Mock Data - Service

## Introduction

The `service_mock` plugin is a utility for generating simulated Service-type input data. It allows adjusting parameters to obtain different mock inputs. [Source code](https://github.com/alibaba/ilogtail/blob/main/plugins/input/mockd/input_mockd.go)

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, Required, no default (e.g., `service_mock`) | Plugin type, always set to `service_mock` |
| Tags | Map, Key-value pairs of String, `{}` | Add custom tags to the mock data as needed. |
| Fields | Map, Key-value pairs of String, `{}` | Add custom fields to the mock data as needed. |
| File | String, `""` | Specify a file to read, adds a field named `content` with the file's contents. |
| Index | Long, `0` | Starting index for generated mock data (incrementing from the next index). |
| LogsPerSecond | Integer, `0` | Number of logs to generate per second. |
| MaxLogCount | Integer, `0` | Maximum number of logs to produce; 0 indicates no limit. |

## Examples

* Configuration Example

```yaml
enable: true
inputs:
  - Type: service_mock
    Index: 100
    Tags:
      tag1: tag1
      tag2: tag2
    Fields:
      field1: field1
      field2: field2
    LogsPerSecond: 2
    MaxLogCount: 3
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output Example

```json
{
  "tag1": "tag1",
  "tag2": "tag2",
  "field1": "field1",
  "field2": "field2",
  "Index": "101",
  "__time__": "1658814793"
}
{
  "tag1": "tag1",
  "tag2": "tag2",
  "field1": "field1",
  "field2": "field2",
  "Index": "102",
  "__time__": "1658814793"
}
{
  "tag1": "tag1",
  "tag2": "tag2",
  "field1": "field1",
  "field2": "field2",
  "Index": "103",
  "__time__": "1658814794"
}
```
