# Renaming Field Names

## Introduction

The `processor_rename processor` plugin allows you to rename fields in log messages.

## Supported Event Types

| LogGroup(v1) | EventTypeLogging | EventTypeMetric | EventTypeSpan |
| ------------ | ---------------- | --------------- | ------------- |
|      ✅      |      ✅           | ✅ Rename Tag    | ✅ Rename Tag   |

## Stability

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter       | Type     | Required | Description                                                                                     |
| -------------- | -------- | -------- | ------------------------------------------------------------------------------------------------- |
| Type           | String   | Yes      | Plugin type.                                                                                     |
| NoKeyError     | Boolean  | No       | Whether to throw an error if no matching field is found. If not provided, defaults to false (no error). |
| SourceKeys     | String[] | Yes      | Original fields to be renamed.                                                                   |
| DestKeys       | String[] | Yes      | Renamed field names.                                                                              |

## Example

Collect logs from the `/home/test-log/` directory, parse JSON-formatted `json.log` files, and then rename the fields in the parsed data.

* Input

```bash
echo '{"key1": 123456, "key2": "abcd"}' >> /home/test-log/json.log
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
  - Type: processor_rename
    SourceKeys:
      - key1
    DestKeys:
      - new_key1
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/key_value.log",
    "new_key1": "123456",
    "key2": "abcd",
    "__time__": "1657354602"
}
```
