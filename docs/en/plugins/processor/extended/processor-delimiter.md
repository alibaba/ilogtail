# Separator

## Introduction

The `processor_split_char processor` plugin extracts fields using a single-character delimiter, supporting the use of quotes around the delimiter.

The `processor_split_string processor` plugin, on the other hand, extracts fields using a multi-character delimiter, which does not support the use of quotes around the delimiter.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

### `processor_split_char` Configuration

| Parameter         | Type       | Required | Description                                                                                     |
| ----------------- | -------- | ---- | ----------------------------------------------- |
| Type              | String   | Yes    | Plugin type                                      |
| SourceKey         | String   | Yes    | Original field name                               |
| SplitSep          | String   | Yes    | Delimiter. Must be a single character, such as \u0001.                                            |
| SplitKeys         | String[] | Yes    | Field names after splitting the log, e.g., ["ip", "time", "method"]                            |
| QuoteFlag         | Boolean  | No     | Whether to use quotes. If not provided, defaults to false, indicating no quotes.                 |
| Quote             | String   | No     | Only valid when QuoteFlag is set to true.                                                      |
| NoKeyError        | Boolean  | No     | Whether to throw an error if no matching original field is found. Defaults to false.         |
| NoMatchError      | Boolean  | No     | Whether to throw an error if the delimiter does not match. Defaults to false.                |
| KeepSource        | Boolean  | No     | Whether to keep the original field. Defaults to false.                                      |

### `processor_split_string` Configuration

| Parameter              | Type       | Required | Description                                                                                   |
| ------------------- | -------- | ---- | --------------------------------------------- |
| Type                | String   | Yes    | Plugin type                                      |
| SourceKey           | String   | Yes    | Original field name                              |
| SplitSep            | String   | Yes    | Delimiter. Can be an invisible character, e.g., \u0001\u0002.                                      |
| SplitKeys           | String[] | Yes    | Field names after splitting the log, e.g., ["ip", "time", "method"]                            |
| PreserveOthers      | Boolean  | No     | Whether to retain extra parts if the field length exceeds the SplitKeys field lengths. Defaults to false. |
| ExpandOthers        | Boolean  | No     | Whether to parse the extra parts. Defaults to false.                                          |
| ExpandKeyPrefix     | String   | No     | Prefix for names of extra parts, e.g., "expand_" for keys like expand_1, expand_2.                  |
| NoKeyError          | Boolean  | No     | Whether to throw an error if no matching original field is found. Defaults to false.         |
| NoMatchError        | Boolean  | No     | Whether to throw an error if the delimiter does not match. Defaults to false.                |
| KeepSource          | Boolean  | No     | Whether to keep the original field. Defaults to false.                                      |

### Example

Collect the `delimiter.log` file in the `/home/test-log/` directory, using the vertical bar (`|`) as the delimiter to extract log field values.

* Input

```bash
echo "127.0.0.1|10/Aug/2017:14:57:51 +0800|POST|PutData?Category=YunOsAccountOpLog|0.024|18204|200|37|-|aliyun-sdk-java" >> /home/test-log/delimiter.log
```

* Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
processors:
  - Type: processor_split_char
    SourceKey: content
    SplitSep: "|"
    SplitKeys:
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
    "__tag__:__path__": "/home/test-log/delimiter.log",
    "ip": "127.0.0.1",
    "time": "10/Aug/2017:14:57:51 +0800",
    "method": "POST",
    "url": "PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1657361070"
}
```
