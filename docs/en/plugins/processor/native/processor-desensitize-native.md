# Native Desensitization Processor

## Introduction

The `processor_filter_regex_native` plugin filters events based on the content of event fields.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  Yes  |  /  |  Plugin type. Fixed as `processor_desensitize_native`.  |
|  SourceKey  |  string  |  Yes  |  /  |  Source field name.  |
|  Method  |  string  |  Yes  |  /  |  Desensitization method. Available options are:<ul><li>const: Replace sensitive content with a constant string.</li><li>md5: Replace sensitive content with its MD5 hash.</li></ul>       |
|  ReplacingString  |  string  |  No (required when Method is const)  |  /  |  Constant string to replace sensitive content with.  |
|  ContentPatternBeforeReplacedString  |  string  |  Yes  |  /  |  Regular expression pattern for the sensitive content's prefix.  |
|  ReplacedContentPattern  |  string  |  Yes  |  /  |  Regular expression pattern for the sensitive content itself.  |
|  ReplacingAll  |  bool  |  No  |  true  |  Whether to replace all occurrences of the sensitive content.  |

## Example

Collect the log file `/home/test-log/sen.log`, and replace the password field with asterisks (******), and output the result to stdout.

- Input

```json
{
  "account": "1812213231432969",
  "password": "04a23f38"
}
```

- Collection Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/reg.log
processors:
  - Type: processor_desensitize_native
    SourceKey: content
    Method: const
    ReplacingString: '******'
    ContentPatternBeforeReplacedString: 'password":'
    ReplacedContentPattern: '[^"]+'
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

- Output

```json
{
  "__tag__:__path__": "/home/test-log/reg.log",
  "content": "{\"account\":\"1812213231432969\",\"password\":\"******\"}",
  "__time__": "1657161810"
}
```
