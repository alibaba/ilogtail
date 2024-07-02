# Regular Expressions

## Introduction

The `processor_regex processor` plugin allows for field extraction from text log entries using pattern matching with regular expressions.

Note: When used as the first log parsing plugin, it is recommended to use the [Regex Accelerator](../accelerator/regex-accelerate.md) plugin instead for better performance.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter        | Type       | Required | Description                                                                                     |
| ---------------- | -------- | ------- | ------------------------------------------------------------------------------------------------- |
| Type             | String    | Yes     | Plugin type                                                                                       |
| SourceKey        | String    | Yes     | Original field name                                                                               |
| Regex            | String    | Yes     | Regular expression, with () to denote fields to extract.                                            |
| Keys             | String[]  | Yes     | Extracted field names, e.g., `["ip", "time", "method"]`.                                          |
| NoKeyError       | Boolean   | No      | Whether to throw an error if no matching original field is found. Defaults to `false` if not provided. |
| NoMatchError     | Boolean   | No      | Whether to throw an error if the regex does not match the original field value. Defaults to `false` if not provided. |
| KeepSource       | Boolean   | No      | Whether to retain the original field. Defaults to `false` if not provided.                            |
| FullMatch        | Boolean   | No      | Defaults to `true`, meaning only fields that fully match the `Regex` parameter's regex are extracted. Set to `false` to extract on partial match. |
| KeepSourceIfParseError | Boolean | No      | Whether to keep the original log on parse error. Defaults to `true` if not provided.                |

## Example

Collect logs from the `/home/test-log/` directory, specifically the `reg.log` file, and extract fields from the log entries。

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
      - /home/test-log/*.log
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
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test-log/reg.log",
    "ip": "127.0.0.1",
    "time": "10/Aug/2017:14:57:51",
    "method": "POST",
    "url": "/PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1657362166"
}
```
