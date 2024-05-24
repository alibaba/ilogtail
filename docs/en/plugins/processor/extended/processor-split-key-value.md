# Key-Value Pairs

## Introduction

The `processor_split_key_value` processor can extract fields by splitting key-value pairs in a document.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter         | Type    | Required | Description                                                                                                                  |
|--------------------|---------|----------|------------------------------------------------------------------------------------------------------------------------------|
| Type               | String  | Yes      | Plugin type.                                                                                                                  |
| SourceKey          | String  | No       | Original field name.                                                                                                          |
| Delimiter          | String  | No       | Separator between key-value pairs. If not provided, defaults to a tab character (\t).                                          |
| Separator          | String  | No       | Separator within a single key-value pair, between the key and the value. If not provided, defaults to a colon (:) character.         |
| KeepSource         | Boolean | No       | Whether to keep the original field. If not provided, defaults to true, meaning the original field is kept.                            |
| ErrIfKeyIsEmpty    | Boolean | No       | Whether to issue an error if the key is an empty string. If not provided, defaults to true, meaning an error is raised.                  |
| EmptyKeyPrefix     | String  | No       | Prefix for the key when it's an empty string. Defaults to "empty_key_". The final key format will be prefix + sequence number, e.g., "empty_key_0". |
| DiscardWhenSeparatorNotFound | Boolean | No       | Whether to discard the key-value pair if no separator is found. If not provided, defaults to false, meaning the pair is not discarded. |
| NoSeparatorKeyPrefix | Boolean | No       | If a key-value pair is discarded when no separator is found, this parameter allows setting a prefix for the key when kept, defaults to "no_separator_key_". The final saved format will be prefix + sequence number: error key-value pair, e.g., "no_separator_key_0": "error key-value pair" |
| ErrIfSourceKeyNotFound | Boolean | No       | Whether to issue an error if the source key is not found. If not provided, defaults to true, meaning an error is raised.                    |
| ErrIfSeparatorNotFound | Boolean | No       | Whether to issue an error if the specified separator (Separator) is not found. If not provided, defaults to true, meaning an error is raised. |
| Quote              | String  | No       | Quotation character. If enabled, extracts the value if it's enclosed by the specified character(s). Note that if the quote character is double quotes, an escape character (\) is needed. If the quote character contains a backslash (\) within it, it's treated as part of the value. Multi-character quotes are supported. By default, this feature is not enabled. |

## Example 1: Splitting Key-Value Pairs

### Splitting Key-Value Pairs 1

Parse the `key_value.log` file in `/home/test-log/` directory, following a log format where key-value pairs are separated by a tab character (\t) and the key-value separator within each pair is a colon (:).

* Input

```bash
echo -e 'class:main\tuserid:123456\tmethod:get\tmessage:"wrong user"' >> /home/test-log/key_value.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_split_key_value
    SourceKey: content
    Delimiter: "\t"
    Separator: ":"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/key_value.log",
    "class": "main",
    "userid": "123456",
    "method": "get",
    "message": "\"wrong user\"",
    "__time__": "1657354602"
}
```

### Splitting Key-Value Pairs with Quotation Marks

Parse the `key_value.log` file in `/home/test-log/`, following the same log format as above but with quotation marks for key-value pairs.

* Input

```bash
echo -e 'class:main http_user_agent:"User Agent" "中文" "hello\"ilogtail\"world"' >> /home/test-log/key_value.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_split_key_value
    SourceKey: content
    Delimiter: " "
    Separator: ":"
    Quote: "\""
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/key_value.log",
    "class": "main",
    "http_user_agent": "User Agent",
    "no_separator_key_0": "中文",
    "no_separator_key_1": "hello\"ilogtail\"world",
    "__time__": "1657354602"
}
```

### Splitting Key-Value Pairs with Multi-Character Quotation Marks

Parse the `key_value.log` file in `/home/test-log/`, using the same log format as before but with multi-character quotation marks for key-value pairs.

* Input

```bash
echo -e 'class:main http_user_agent:"""User Agent""" "中文"' >> /home/test-log/key_value.log
```

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_split_key_value
    SourceKey: content
    Delimiter: " "
    Separator: ":"
    Quote: "\"\"\""
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "__tag__:__path__": "/home/test_log/key_value.log",
    "class": "main",
    "http_user_agent": "User Agent",
    "no_separator_key_0": "中文",
    "__time__": "1657354602"
}
```
