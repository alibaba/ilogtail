# Original Data

## Introduction

The `processor_default` plugin performs no data manipulation; it simply passes through the data. [Source Code](https://github.com/alibaba/ilogtail/blob/main/plugins/processor/defaultone/processor_default.go)

## Supported Event Types

| **LogGroup(v1)** | **EventTypeLogging** | **EventTypeMetric** | **EventTypeSpan** |
| --- | --- | --- | --- |
| ✅ | ✅ | ✅ | ✅ |

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, Required (no default) | Plugin type, always set to `processor_default` |

## Example

Collect data from the `/home/test-log/` directory, specifically the `default.log` file, and extract the raw data from the file.

* Input

```bash
echo "2022/07/14/11:32:47 test log" >> /home/test-log/default.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_default
    SourceKey: content
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
  "__tag__:__path__": "/home/test-log/default.log",
  "content": "2022/07/14/11:32:47 test log",
  "__time__": "1657769827"
}
```
