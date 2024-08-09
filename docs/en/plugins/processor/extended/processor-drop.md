# Dropping Fields

## Introduction

The `processor_drop processor` plugin allows you to discard specific log fields from the logs it processes.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter     | Type     | Required | Description                             |
| ------------- | -------- | -------- | ----------------------------------------- |
| Type          | String   | Yes      | The plugin type.                         |
| DropKeys      | String[] | Yes      | List of fields to be discarded, supports multiple keys. |

## Example

采集`/home/test-log/`路径下的`json.log`文件，解析日志内容为`Json`格式，然后丢弃指定的字段。

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
  - Type: processor_drop
    DropKeys: 
      - key1
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test-dir/test_log/json.log",
    "key2": "abcd",
    "__time__": "1657354602"
}
