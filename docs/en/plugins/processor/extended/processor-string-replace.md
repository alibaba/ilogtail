# String Replacement

## Introduction

The `processor_string_replace` processor enables full-text, regex-based, or unescaping string replacement in log text from your data sources.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter        | Type       | Required | Description                                                                                     |
| ---------------- | -------- | ---- | ----------------------------------------------- |
| Type             | String   | Yes   | Plugin type (e.g., "processor_string_replace") |
| SourceKey        | String   | Yes   | Field name to match content against.            |
| Method            | String | Yes  | No default. Available options are:<br>const: Full-text replacement.<br>regex: Regular expression matching and replacement.<br>unquote: Unescaping. |
| Match             | String | No    | No default. Values can be:<br>const: The string to match.<br>regex: A regular expression to match, with multiple matches replacing all, or use groups for specific replacements.<br>unquote: No input needed for unescaping. |
| ReplaceString    | String  | No    | Default is "". Values can be:<br>const: The string to replace matches with.<br>regex: A string for replacement, supporting group replacement.<br>unquote: No input needed for unescaping. |
| DestKey           | String  | No    | No default. Field to store the replaced string in, if not the original field. |

## Examples

### Example 1: Full-text Match and Replacement

Collect the `string_replace.log` file under `/home/test-log/`, demonstrating log content's regex matching and replacement functionality.

* Input

```bash
echo 'hello,how old are you? nice to meet you' >> /home/test-log/string_replace.log
```

* Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
processors:
  - Type: processor_string_replace
    SourceKey: content
    Method: const
    Match: 'how old are you?'
    ReplaceString: ''
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/string_replace.log",
    "content": "hello, nice to meet you",
    "__time__": "1680353730"
}
```

### Example 2: Basic Regular Expression Match and Replacement

Collect the `string_replace.log` file under `/home/test-log/`, testing log content's regular expression matching and replacement.

* Input

```bash
echo '2022-09-16 09:03:31.013 \u001b[32mINFO \u001b[0;39m \u001b[34m[TID: N/A]\u001b[0;39m [\u001b[35mThread-30\u001b[0;39m] \u001b[36mc.s.govern.polygonsync.job.BlockTask\u001b[0;39m : 区块采集------结束------\r' >> /home/test-log/string_replace.log
```

* Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
processors:
  - Type: processor_string_replace
    SourceKey: content
    Method: regex
    Match: \\u\w+\[\d{1,3};*\d{1,3}m|N/A
    ReplaceString: ''
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/string_replace.log",
    "content": "2022-09-16 09:03:31.013 INFO  [TID: ] [Thread-30] c.s.govern.polygonsync.job.BlockTask : 区块采集------结束------\r",
    "__time__": "1680353730"
}
```

### Example 3: Group-based Matching and Replacement, with Output to a New Field

Collect the `string_replace.log` file under `/home/test-log/`, demonstrating log content's regular expression matching with group-based replacement.

* Note: In ReplaceString, curly braces `{}` are not allowed for group references. Use `$1`, `$2`, etc. for referencing groups.

* Input

```bash
echo '10.10.239.16' >> /home/test-log/string_replace.log
```

* Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
processors:
  - Type: processor_string_replace
    SourceKey: content
    Method: regex
    Match: (\d.*\.)\d+
    ReplaceString: $1*/24
    DestKey: new_ip
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/string_replace.log",
    "content": "10.10.239.16",
    "new_ip": "10.10.239.*/24",
    "__time__": "1680353730"
}
```

### Example 4: Escaped Character Replacement

Collect the `string_replace.log` file under `/home/test-log/`, testing escaped character replacement functionality.

* Input

```bash
echo '{\\x22UNAME\\x22:\\x22\\x22,\\x22GID\\x22:\\x22\\x22,\\x22PAID\\x22:\\x22\\x22,\\x22UUID\\x22:\\x22\\x22,\\x22STARTTIME\\x22:\\x22\\x22,\\x22ENDTIME\\x22:\\x22\\x22,\\x22UID\\x22:\\x222154212790\\x22,\\x22page_num\\x22:1,\\x22page_size\\x22:10}' >> /home/test-log/string_replace.log
echo '\\u554a\\u554a\\u554a' >> /home/test-log/string_replace.log
```

* Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
processors:
  - Type: processor_string_replace
    SourceKey: content
    Method: unquote
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/string_replace.log",
    "content": "{\"UNAME\":\"\",\"GID\":\"\",\"PAID\":\"\",\"UUID\":\"\",\"STARTTIME\":\"\",\"ENDTIME\":\"\",\"UID\":\"2154212790\",\"page_num\":1,\"page_size\":10}",
    "__time__": "1680353730"
}

{
    "__tag__:__path__": "/home/test_log/string_replace.log",
    "content": "啊啊啊",
    "__time__": "1680353730"
}
```
