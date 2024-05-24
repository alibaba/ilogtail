# Basic Aggregation

## Introduction

The `aggregator_content_value_group` `aggregator` plugin allows for aggregating log entries based on specific keys specified per entry.

## Version

[Alpha](../stability-level.md)

## Configuration Parameters

| Parameter         | Type     | Required | Description                                                                                                          |
| ----------------- | -------- | -------- | -------------------------------------------------------------------------------------------------------------------- |
| Type              | String   | Yes      | Plugin type, set to `aggregator_content_value_group`.                                                                       |
| GroupKeys         | []String | Yes      | List of keys to group by their values.                                                                                     |
| EnablePackID      | Boolean  | No       | Whether to add the `__pack_id__` field to the LogTag within the LogGroup. If not provided, it defaults to adding the field. |
| Topic             | String   | No       | Topic name for the LogGroup. If not provided, each LogGroup's Topic name defaults to an empty string.                            |
| ErrIfKeyNotFound  | Boolean  | No       | Whether to print an error log if the specified key is not found in the Log's Contents.                                   |

## Example

Collect all filenames matching the `reg.log` pattern from the `/home/test-log/` directory, then extract fields using `processor_regex`, and aggregate the results based on the `url` and `method` fields, sending the output to SLS.

* Input

```bash
echo '127.0.0.1 - - [10/Aug/2017:14:57:51 +0800] "POST /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"' >> /home/test-log/reg.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/reg.log
processors:
  - Type: processor_regex
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
aggregators:
  - Type: aggregator_content_value_group
    GroupKeys:
      - url
      - method
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```
