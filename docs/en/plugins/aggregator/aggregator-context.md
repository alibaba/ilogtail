# Context Aggregation

## Introduction

The `aggregator_context` plugin allows for aggregating individual log entries based on their source in a single log record.

## Version

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter | Type | Required | Description |
| --- | --- | --- | --- |
| Type | String | Yes | Plugin type, set to `aggregator_context`. |
| MaxLogGroupCount | Int | No | Maximum number of LogGroups allowed per log source before a flush. If not provided, the default is 2 LogGroups per source. |
| MaxLogCount | Int | No | Maximum number of logs per LogGroup. If not provided, the default is 1024 logs per LogGroup. |
| PackFlag | Boolean | No | Whether to add a `__pack_id__` field in the LogTag of the LogGroup. If not provided, a `__pack_id__` field is added by default. |
| Topic | String | No | Custom topic name for the LogGroup. If not provided, the default topic name is: <li>Empty if the input plugin does not support setting a topic;<li>The topic name set in the input plugin if it supports setting a topic. |

## Example

Collect all files matching the pattern `*.log` under `/home/test-log/`, and use context aggregation to group logs from the same file together. The aggregated results are sent to SLS.

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
aggregators:
  - Type: aggregator_context
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```
