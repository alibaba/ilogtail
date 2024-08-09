# Adding Fields

## Introduction

The `processor_add_fields processor` plugin allows you to add custom fields to log entries.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter        | Type    | Required | Description                                                                                     |
| ----------------- | ------- | -------- | ------------------------------------------------------------------------------------------------- |
| Type              | String  | Yes      | Plugin type.                                                                                     |
| Fields            | Map     | Yes      | The fields to be added and their values. Key-value pairs format, supports multiple additions.         |
| IgnoreIfExist     | Boolean | No       | Whether to ignore the field if it already exists. If not provided, defaults to `false`, meaning not to ignore. |

## Examples

Collect the `test.log` file from `/home/test-log/` and add the field `service: A` to indicate the service the log belongs to.

* Input

```bash
echo 'this is a test log' >> /home/test-log/test.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_add_fields
    Fields:
      service: A
    IgnoreIfExist: false
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/key_value.log",
    "content": "this is a test log",
    "service": "A",
    "__time__": "1657354602"
}
```
