# 日志限速

## 简介

`processor_rate_limit processor`插件可以实现对日志的限速。在限定时间内，相同索引值下的日志只有低于等于限定速率数量的日志才会被采集，否则直接丢弃。

## 版本

[Stable](../stability-level.md)

## 配置参数

| 参数                     | 类型，默认值 | 说明                                                |
| ---------------------- | ------- | ------------------------------------------------- |
| Fields                | []string，`[]` | 限速的索引字段。processor会根据这些字段的值所组合得到的结果，进行分别限速。|
| Limit                | string，`[]` | 限速速率。格式为 `数字/时间单位`。支持的时间单位为 `s`（每秒），`m`（每分钟），`h`（每小时）
|

## 样例

* 输入

```bash
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "aliyun-sdk-java"}' >> /home/test-log/proccessor-rate-limit.log
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "aliyun-sdk-java"}' >> /home/test-log/proccessor-rate-limit.log
echo '{"ip": "10.**.**.**", "method": "POST", "brower": "aliyun-sdk-java"}' >> /home/test-log/proccessor-rate-limit.log
```

* 采集配置

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

* 输出

```json
{
  "__tag__:__path__": "/home/test-log/proccessor-rate-limit.log",
  "__time__": "1658837955",
  "brower": "aliyun-sdk-java",
  "ip": "10.**.**.**",
  "method": "POST"
}
```
