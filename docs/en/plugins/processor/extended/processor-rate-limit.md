# Log Rate Limit

## Introduction

The `processor_rate_limit processor` plugin is designed to throttle log processing, ensuring that within a specified time window, the number of log entries with the same indexed value does not exceed the predefined rate limit. Excess entries for a particular index will be discarded, preventing their collection.

For example, if "ip" is the index field, consider two logs: `{"ip": "10.**.**.**", "method": "POST", "browser": "aliyun-sdk-java"}` and `{"ip": "10.**.**.**", "method": "GET", "browser": "aliyun-sdk-c++"}`. Both logs have the same "ip" index value ("10..."). The processor will aggregate all logs with the "ip" of "10..." and enforce the rate limit within the given time frame.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter                     | Type, Default | Description                                                  |
| ---------------------- | ------- | ------------------------------------------------------------- |
| Fields                | []string, `[]` | Index fields to limit. The processor will apply rate limits separately based on the combinations of these fields.|
| Limit                | string, `[]` | Rate limit in the format `number/time unit`. Supported time units are `s` (seconds), `m` (minutes), `h` (hours). |

## Example

* Input

```bash
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "aliyun-sdk-java"}' >> /home/test-log/proccessor-rate-limit.log
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "aliyun-sdk-java"}' >> /home/test-log/proccessor-rate-limit.log
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "aliyun-sdk-java"}' >> /home/test-log/proccessor-rate-limit.log
```

* Collector Configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
processors:
  - Type: processor_json
    SourceKey: content
    KeepSource: false
    ExpandDepth: 1
    ExpandConnector: ""
  - Type: processor_rate_limit
    Fields:
      - "ip"
    Limit: "1/s"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
  "__tag__:__path__": "/home/test-log/proccessor-rate-limit.log",
  "__time__": "1658837955",
  "brower": "aliyun-sdk-java",
  "ip": "10.**.**.**",
  "method": "POST"
}
```
