# Multiline Split

## Introduction

The `processor_split_log_regex processor` plugin is designed to collect and process multi-line logs, such as those from Java applications, by splitting them into separate entries.

**Note:** It is recommended to use the [Regex Accelerator](../accelerator/regex-accelerate.md) or [Delimiter Accelerator](../accelerator/delimiter-accelerate.md) plugins for multiline splitting, as they offer better performance.

When used alone or without acceleration, this plugin must be the first processor in the chain.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter         | Type      | Required | Description                                                                                     |
| ---------------- | ------- | ---- | ----------------------------------------------- |
| Type             | String  | Yes   | Plugin type.                                                                                     |
| SplitKey         | String  | Yes   | The field to base the split on.              |
| SplitRegex       | String  | Yes   | <p>A regex pattern at the beginning of each line to identify the start of a multi-line log block. Default is `.*`, which splits every line.</p> |
| PreserveOthers   | Boolean | No    | Whether to retain other fields not in the SplitKey. |
| NoKeyError       | Boolean | No    | Whether to throw an error if no match for the SplitKey is found. If not specified, defaults to `false` (no error). |

## Example

采集`/home/test-log/`目录下的`multiline.log`文件，并按行首正则进行多行切分。

* Input

```bash
echo -e '[2022-03-03 18:00:00] xxx1\nyyyyy\nzzzzzz\n[2022-03-03 18:00:01] xxx2\nyyyyy\nzzzzzz' >> /home/test-log/multiline.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_split_log_regex
    SplitRegex: \[\d+-\d+-\d+\s\d+:\d+:\d+\] \.*
    SplitKey: content
    PreserveOthers: true
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test-log/multiline.log",
    "content": "[2022-03-03 18:00:00] xxx1\nyyyyy\nzzzzzz\n",
    "__time__": "1657367638"
}

{
    "__tag__:__path__": "/home/test-log/multiline.log",
    "content": "[2022-03-03 18:00:01] xxx2\nyyyyy\nzzzzzz",
    "__time__": "1657367638"
}
```
