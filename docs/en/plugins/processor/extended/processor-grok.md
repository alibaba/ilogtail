# Grok

## Introduction

The `processor_grok` plugin allows for field extraction from text logs using Grok patterns. [Source code](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/processor_grok.go)

## Version

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, No default (required) | Plugin type, always set to `processor_grok` |
| CustomPatternDir | Array of Strings, `[]` | Directory containing custom Grok patterns files, which are read from all files in the directory. Requires a restart for changes to take effect. |
| CustomPatterns | Map of Strings, `{}` | Custom Grok patterns, with keys as pattern names and values as Grok expressions. |
| SourceKey | String, `content` | Target field to match. |
| Match | Array of Strings, `[]` | Array of Grok patterns to try against the log, processing them from top to bottom. Multiple matches can affect performance; see [Efficiency and Optimization](#efficiency-and-optimization) for details. |
| TimeoutMilliSeconds | Long, `0` | Maximum time to attempt parsing a Grok expression, in milliseconds. Set to 0 to disable timeouts. |
| IgnoreParseFailure | Boolean, `true` | Whether to ignore failures in parsing, or to fill the `content` field with the returned value if parsing is disabled. |
| KeepSource | Boolean, `true` | Whether to keep the original field. |
| NoKeyError | Boolean, `false` | Whether to raise an error if no match for the original field is found. |
| NoMatchError | Boolean, `true` | Whether to raise an error if none of the patterns in `Match` match. |
| TimeoutError | Boolean, `true` | Whether to return an error if the timeout is reached. |

## Examples

### Collecting Logs

Collect logs from `/home/test-log/` directory, using the provided configuration options to extract log information.

* Configuration Example

```yaml
enable: true
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
processors:
  - Type: processor_grok
    SourceKey: content
    KeepSource: false
    CustomPatterns:
      HTTP: '%{IP:client} %{WORD:method} %{URIPATHPARAM:request} %{NUMBER:bytes} %{NUMBER:duration}'
    Match: 
      - '%{HTTP}'
      - '%{WORD:word1} %{NUMBER:request_time} %{WORD:word2}'
      - '%{YEAR:year} %{MONTH:month} %{MONTHDAY:day} %{QUOTEDSTRING:motto}'
    IgnoreParseFailure: false
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

### Input 1

```bash
echo 'begin 123.456 end' >> /home/test-ilogtail/test-log/processor-grok.log
```

### Output 1

```json
{
  "__tag__:__path__": "/home/test-ilogtail/test-log/processor-grok.log",
  "word1": "begin",
  "request_time": "123.456",
  "word2": "end",
  "__time__": "1662618045"
}
```

### Input 2

```bash
echo '2019 June 24 "I am iron man"' >> /home/test-ilogtail/test-log/processor-grok.log
```

### Output 2

```json
{
  "__tag__:__path__": "/home/test-ilogtail/test-log/processor-grok.log",
  "year": "2019",
  "month": "June",
  "day": "24",
  "motto": "\"I am iron man\"",
  "__time__": "1662618059"
}
```

### Input 3

```bash
echo 'WRONG LOG' >> /home/test-ilogtail/test-log/processor-grok.log
```

### Output 3

```json
{
  "__tag__:__path__": "/home/test-ilogtail/test-log/processor-grok.log",
  "__time__": "1662618069"
}
```

### Input 4

```bash
echo '10.0.0.0 GET /index.html 15824 0.043' >> /home/test-ilogtail/test-log/processor-grok.log
```

### Output 4

```json
{
  "__tag__:__path__": "/home/test-ilogtail/test-log/processor-grok.log",
  "client": "10.0.0.0",
  "method": "GET",
  "request": "/index.html",
  "bytes": "15824",
  "duration": "0.043",
  "__time__": "1662618081"
}
```

### Output Explanation

In Example 1, the `processor_grok` plugin first tries the first expression in `Match` (`'%{HTTP}'`), and if it fails, it proceeds to the next one. It then successfully matches the second expression (`'%{WORD:word1} %{NUMBER:request_time} %{WORD:word2}'`), and since `KeepSource` is set to `false`, the original `content` field is discarded.

Examples 2 and 4 are similar to Example 1, using the third and first expressions in `Match` respectively.

In Example 3, the `processor_grok` plugin tries all three expressions in `Match` and fails, resulting in no output. Since `IgnoreParseFailure` is set to `false`, the log with no match is discarded.

## Grok Patterns

The `processor_grok` plugin primarily relies on [SLS's Grok patterns](https://help.aliyun.com/document_detail/129387.html). SLS Data Processing provides over 70 commonly used Grok patterns, such as for ID cards, email addresses, MAC addresses, IP addresses, and time parsing. Additional generic Grok patterns can be found [here](https://github.com/alibaba/ilogtail/tree/main/example_config/processor_grok_patterns).

## Efficiency and Optimization

### Regex Engine

Due to limitations in the native `regexp` library in Golang, the `processor_grok` uses a third-party library called [regexp2](https://github.com/dlclark/regexp2). A performance comparison between `regexp2` and the Golang's `regexp` can be found in the [Benchmark Test](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/processor_grok_benchmark_test.go).

### Optimizing Match Failure Time

To improve the plugin's efficiency, several strategies can be employed to optimize the time spent on failed matches:

* Ensure that patterns match the data exactly

* Use anchors in Grok patterns, like `^` and `$`, to minimize unnecessary attempts

* Set a timeout, using the `TimeoutMilliSeconds` parameter

### Optimizing Multiple Matches

The `processor_grok` supports multiple matches, trying multiple Grok patterns against the log in sequence. However, using multiple matches can significantly decrease performance. See the [Benchmark Test](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/processor_grok_benchmark_test.go) for a simple simulation of performance degradation when using 1, 2, 3, or 5 expressions in `Match`.

Therefore, when using the `processor_grok` plugin, it's best to avoid using multiple matches or keep the number of expressions in `Match` to a minimum. You can also reduce redundant matches by using a layered approach, as shown in the following example.

### Optimized Configuration

* Input

Three data entries:

```bash
'8.8.8.8 process-name[666]: a b 1 2 a lot of text at the end'
'8.8.8.8 process-name[667]: a 1 2 3 a lot of text near the end;4'
'8.8.8.8 process-name[421]: a completely different format | 1111'
```

* Standard Configuration (Multiple Matches)

Standard multiple match configuration, trying full expressions one by one.

```yaml
processors:
  - Type: processor_grok
    SourceKey: content
    Match: 
      - '%{IPORHOST:clientip} %{DATA:process_name}\[%{NUMBER:process_id}\]: %{WORD:word_1} %{WORD:word_2} %{NUMBER:number_1} %{NUMBER:number_2} %{DATA:data}'    
      - '%{IPORHOST:clientip} %{DATA:process_name}\[%{NUMBER:process_id}\]: %{WORD:word_1} %{NUMBER:number_1} %{NUMBER:number_2} %{NUMBER:number_3} %{DATA:data};%{NUMBER:number_4}'
      - '%{IPORHOST:clientip} %{DATA:process_name}\[%{NUMBER:process_id}\]: %{DATA:data} \| %{NUMBER:number}'
```

* Optimized Configuration (Layered Approach)

First, handle the first part uniformly, then the second part:

```yaml
processors:
  - Type: processor_grok
    SourceKey: content
    Match: 
      - '%{IPORHOST:clientip} %{DATA:process_name}\[%{NUMBER:process_id}\]: %{GREEDYDATA:content_2}'
    KeepSource: false
  - Type: processor_grok
    SourceKey: content_2
    Match: 
      - '%{WORD:word_1} %{WORD:word_2} %{NUMBER:number_1} %{NUMBER:number_2} %{GREEDYDATA:data}'
      - '%{WORD:word_1} %{NUMBER:number_1} %{NUMBER:number_2} %{NUMBER:number_3} %{DATA:data};%{NUMBER:number_4}'
      - '%{DATA:data} \| %{NUMBER:number}'
    KeepSource: false
```
