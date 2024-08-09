# Debug Text Log

## Introduction

The `metric_debug_file` plugin is a debugging tool that reads content line by line from a specified file, binds it with a given name, and emulates file input for monitoring purposes. [Source code](https://github.com/alibaba/ilogtail/blob/main/plugins/input/debugfile/input_debug_file.go)

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter                | Type      | Required | Description                                                                                     |
| ----------------- | ------- | ---- | ----------------------------------------------- |
| Type              | String  | Yes   | Plugin type, always set to `metric_debug_file`.                                          |
| InputFilePath     | String  | Yes   | Path of the file to read.                      |
| FieldName         | String  | No    | Name of the generated field. Default is `content`.                            |
| LineLimit         | Integer | No    | Limit to read the first n lines. Default is 1000 lines. |

## Example

* Input

```shell
echo "2022/07/26/14:25:47 hello world" >> /home/test-log/debug_file.log
echo "2023/05/23/19:37:47 hello world" >> /home/test-log/debug_file.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: metric_debug_file
    InputFilePath: /home/test-log/debug_file.log
    FieldName: HelloWorld
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "HelloWorld": "2022/07/26/14:25:47 hello world",
    "__time__": "1658814799"
}

{
    "HelloWorld": "2023/05/23/19:37:47 hello world",
    "__time__": "1658814799"
}
```
