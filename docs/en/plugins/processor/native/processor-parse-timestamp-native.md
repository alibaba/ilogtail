# Native Timestamp Parsing Processor

## Introduction

The `processor_parse_timestamp_native` plugin parses the timestamp field in event logs and sets the result as the `__time__` field of the event.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| Type | string | Yes | / | Plugin type. Fixed as `processor_parse_timestamp_native`. |
| SourceKey | string | Yes | / | Name of the source field containing the timestamp. |
| SourceFormat | string | Yes | / | Log timestamp format. See Table 1 for details. |
| SourceTimezone | string | No | None | Timezone of the log timestamp in GMT+HH:MM (East) or GMT-HH:MM (West) format. |

- Table 1: Time Format

| **Time Format** | **Description** | **Example** |
| --- | --- | --- |
| `%a` | Abbreviated weekday. | Fri |
| `%A` | Full weekday name. | Friday |
| `%b` | Abbreviated month name. | Jan |
| `%B` | Full month name. | January |
| `%d` | Day of the month, decimal, range 01-31. | 07, 31 |
| `%h` | Abbreviated month name, equivalent to `%b`. | Jan |
| `%H` | Hours in 24-hour format. | 22 |
| `%I` | Hours in 12-hour format. | 11 |
| `%m` | Month, decimal, range 01-12. | 08 |
| `%M` | Minutes, decimal, range 00-59. | 59 |
| `%n` | Newline character. | Newline character |
| `%p` | AM or PM. | AM, PM |
| `%r` | 12-hour time format, equivalent to `%I:%M:%S %p`. | 11:59:59 AM |
| `%R` | Hour and minute combination, equivalent to `%H:%M`. | 23:59 |
| `%S` | Seconds, decimal, range 00-59. | 59 |
| `%t` | Tab character, tab. | None |
| `%y` | Year, decimal, no century, range 00-99. | 04, 98 |
| `%Y` | Year, decimal. | 2004, 1998 |
| `%C` | Century, decimal, range 00-99. | 16 |
| `%e` | Day of the month, decimal, range 1-31. If single-digit, a space is prepended. | 7, 31 |
| `%j` | Day of the year, decimal, range 001-366. | 365 |
| `%u` | Day of the week, decimal, range 1-7, 1 represents Monday. | 2 |
| `%U` | Week of the year, Sunday-start, range 00-53. | 23 |
| `%V` | Week of the year, Monday-start, range 01-53. If the first week of January has >=4 days, it's considered week 1, otherwise the following week is considered week 1. | 24 |
| `%w` | Day of the week, decimal, range 0-6, 0 represents Sunday. | 5 |
| `%W` | Week of the year, Sunday-start, range 00-53. | 23 |
| `%c` | Standard date and time. | Tue Nov 20 14:12:58 2020 |
| `%x` | Standard date, no time. | Tue Nov 20 2020 |
| `%X` | Standard time, no date. | 11:59:59 |
| `%s` | Unix timestamp. | 1476187251 |

## Example

Collect the file `/home/test-log/reg.log`, parse the log content using a regular expression to extract fields, then use the parsed timestamp from the log as the `__time__` field of the event, and finally output the result to stdout.

- Input

```plaintext
127.0.0.1 - - [2023-12-28T08:07:12.187340] "POST /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"
```

- Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/reg.log
processors:
  - Type: processor_parse_regex_native
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
  - Type: processor_parse_timestamp_native
    SourceKey: time
    SourceFormat: '%Y-%m-%dT%H:%M:%S'
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

- Output

```json
{
    "__tag__:__path__": "/home/test-log/reg.log",
    "ip": "127.0.0.1",
    "time": "2023-12-28T08:07:12.187340",
    "method": "POST",
    "url": "/PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1703722032"
}
```
