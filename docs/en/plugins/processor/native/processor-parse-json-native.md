# Native JSON Parsing Processor

## Introduction

The `processor_parse_json_native` plugin parses event fields in JSON format and extracts new fields during event processing.

## Stability

[Stable](../stability-level.md)

## Configuration Parameters

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  Yes  |  /  |  Plugin type. Fixed as processor\_parse\_json\_native.  |
|  SourceKey  |  string  |  Yes  |  /  |  Name of the source field.  |
|  KeepingSourceWhenParseFail  |  bool  |  No  |  false  |  Whether to keep the source field when parsing fails.  |
|  KeepingSourceWhenParseSucceed  |  bool  |  No  |  false  |  Whether to keep the source field when parsing succeeds.  |
|  RenamedSourceKey  |  string  |  No  |  Empty  |  Field name to use when keeping the source field. If not provided, the source field will not be renamed by default.  |

## Example

Collect a JSON-formatted file `/home/test-log/json.log`, parse the log content, extract fields, and output the results to stdout.

### Input

```json
{
  "url": "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1",
  "ip": "10.200.98.220",
  "user-agent": "aliyun-sdk-java",
  "request": {"status": "200", "latency": "18204"},
  "time": "07/Jul/2022:10:30:28"
}
```

### Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/json.log
processors:
  - Type: processor_parse_json_native
    SourceKey: content
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

### Output

```json
{
  "__tag__:__path__": "/home/test-log/json.log",
  "url": "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1",
  "ip": "10.200.98.220",
  "user-agent": "aliyun-sdk-java",
  "request": {
    "status": "200",
    "latency": "18204"
  },
  "time": "07/Jul/2022:10:30:28",
  "__time__": "1657161028"
}
```
