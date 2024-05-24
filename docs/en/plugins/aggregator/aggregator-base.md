# Basic Aggregation

## Introduction

The `aggregator_base` and `aggregator` plugins enable aggregation of individual log entries into groups.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter | Type | Required | Description |
| --- | --- | --- | --- |
| Type | String | Yes | Plugin type, set to `aggregator_base`. |
| MaxLogGroupCount | Int | No | Maximum number of `LogGroup`s allowed in memory before `Flush` is executed. If not specified, the default is 4 `LogGroup`s in memory. |
| MaxLogCount | Int | No | Maximum number of log entries per `LogGroup`. If not provided, the default is 1024 log entries per `LogGroup`. |
| PackFlag | Boolean | No | Whether to add a `__pack_id__` field in the `LogTag` of a `LogGroup`. If not set, a `__pack_id__` field is added by default. |
| Topic | String | No | Topic name for the `LogGroup`. If not provided, the default topic name is empty. |

## Example

Collect all files with filenames matching the pattern `*.log` from the `/home/test-log/` directory, and use the basic aggregation feature. Set the Topic to "file" and send the aggregated results to SLS.

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
aggregators:
  - Type: aggregator_base
    Topic: file
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```
