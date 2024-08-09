# Data Masking

## Introduction

The `processor_desensitize` plugin allows for the anonymization of sensitive data in text log files using regular expressions. [Source code](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/processor_desensitize.go)

## Version

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, Required, No Default | Plugin type, always set to `processor_desensitize` |
| SourceKey | String, Required, No Default | Field name in the log. |
| Method | String, Required, No Default | Masking method. Options include:<br>const: Replace sensitive data with the string specified in the ReplaceString parameter.<br>md5: Replace with the MD5 hash of the sensitive data. |
| Match | String, Required, No Default | Specifies the sensitive data to be masked.<br>full: The entire field content.<br>regex: Use regular expressions to extract sensitive data. |
| ReplaceString | String, Required if Method is "const" | The string to replace sensitive data with, mandatory when Method is set to "const". |
| RegexBegin | String, Required if Match is "regex" | Regular expression to specify the prefix of sensitive data, mandatory when Match is configured as "regex". |
| RegexContent | String, Required if Match is "regex" | Regular expression to specify the content of sensitive data, mandatory when Match is configured as "regex". |

## Example

Collect logs from the `/home/test-log/` directory containing sensitive data in the `processor-desensitize.log` file, using the provided configuration options to extract log information.

* Input

```bash
echo "[{'account':'1812213231432969','password':'04a23f38'}, {account':'1812213685634','password':'123a'}]" >> /home/test-ilogtail/test-log/processor-desensitize.log
```

* Configuration 1

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_desensitize
    SourceKey: content
    Method: "const"
    Match: "full"
    ReplaceString: "********"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output 1

```json
{
  "content": "********",
}
```

* Configuration 2

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_desensitize
    SourceKey: content
    Method: "const"
    Match: "regex"
    ReplaceString: "********"
    RegexBegin: "'password':'"
    RegexContent: "[^']*"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output 2

```json
{
  "content": "[{'account':'1812213231432969','password':'********'}, {'account':'1812213685634','password':'********'}]",
}
```

* Configuration 3

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_desensitize
    SourceKey: content
    Method: "md5"
    Match: "regex"
    ReplaceString: "********"
    RegexBegin: "'password':'"
    RegexContent: "[^']*"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output 3

```json
{
  "content": "[{'account':'1812213231432969','password':'9c525f463ba1c89d6badcd78b2b7bd79'}, {'account':'1812213685634','password':'1552c03e78d38d5005d4ce7b8018addf'}]",
}
```
