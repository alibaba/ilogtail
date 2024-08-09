# Json

## Introduction

The `processor_json processor` plugin is designed to parse logs in JSON format.

**Note:** It is recommended to use the [Json Accelerator](../accelerator/json-accelerate.md) plugin as the first log parser for improved performance.

## Supported Event Types

| LogGroup(v1) | EventTypeLogging | EventTypeMetric | EventTypeSpan |
| ------------ | ---------------- | --------------- | ------------- |
|      ✅      |      ✅           |       ❌        |      ❌       |

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter                     | Type      | Required | Description                                                  |
| ---------------------- | ------- | ---- | ------------------------------------------------------------- |
| Type                   | String  | Yes    | Plugin type                                                  |
| SourceKey              | String  | Yes    | Original field name                                           |
| NoKeyError             | Boolean | No    | Whether to throw an error if no matching field is found. If not provided, defaults to true (throw error). |
| ExpandDepth            | Int     | No    | Depth of JSON expansion. If not provided, defaults to 0 (unlimited). 1 means current level, and so on. |
| ExpandConnector        | String  | No    | Connector used for JSON expansion, can be empty. If not provided, defaults to an underscore (_). |
| Prefix                 | String  | No    | Prefix to attach to fields when expanding JSON. If not provided, defaults to empty. |
| KeepSource             | Boolean | No    | Whether to retain the original field. If not provided, defaults to true (retain). |
| UseSourceKeyAsPrefix   | Boolean | No    | Whether to use the original field name as a prefix for all expanded JSON field names. If not provided, defaults to false. |
| KeepSourceIfParseError | Boolean | No    | Whether to keep the original log if parsing fails. If not provided, defaults to true (keep original log). |
| IgnoreFirstConnector   | Boolean | No    | Whether to ignore the first connector. If not provided, defaults to false. |
| ExpandArray            | Boolean | No    | Whether to expand JSON arrays. If not provided, defaults to false (do not expand arrays). |

## Example

Collect logs from the `/home/test-log/` directory, specifically the `json.log` file, and parse them in JSON format.

* Input

```bash
echo '{"key1": 123456, "key2": "abcd"}' >> /home/test-log/json.log
```

* Collector Configuration

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
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test-dir/test_log/json.log",
    "key1": "123456",
    "key2": "abcd",
    "__time__": "1657354602"
}
```
