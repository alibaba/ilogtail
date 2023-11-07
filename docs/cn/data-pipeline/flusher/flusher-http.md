# 标准输出/文件

## 简介

`flusher_http` `flusher`插件可以实现将采集到的数据，经过处理后，通过http格式发送到指定的地址。

## 版本

[Alpha](../stability-level.md)

## 配置参数

| 参数                           | 类型                 | 是否必选 | 说明                                                                                                                                                                                         |
|------------------------------|--------------------| -------- |--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Type                         | String             | 是       | 插件类型，固定为`flusher_http`                                                                                                                                                                     |
| RemoteURL                    | String             | 是       | 要发送到的URL地址，示例：`http://localhost:8086/write`                                                                                                                                                |
| Headers                      | Map<String,String> | 否       | 发送时附加的http请求header，如可添加 Authorization、Content-Type等信息，支持动态变量写法，如`{"x-db":"%{tag.db}"}`<p>v2版本支持从Group的Metadata或者Group.Tags中获取动态变量，如`{"x-db":"%{metadata.db}"}`或者`{"x-db":"%{tag.db}"}`</p> |
| Query                        | Map<String,String> | 否       | 发送时附加到url上的query参数，支持动态变量写法，如`{"db":"%{tag.db}"}`<p>v2版本支持从Group的Metadata或者Group.Tags中获取动态变量，如`{"db":"%{metadata.db}"}`或者`{"db":"%{tag.db}"}`</p>                                          |
| Timeout                      | String             | 否       | 请求的超时时间，默认 `60s`                                                                                                                                                                           |
| Retry.Enable                 | Boolean            | 否       | 是否开启失败重试，默认为 `true`                                                                                                                                                                        |
| Retry.MaxRetryTimes          | Int                | 否       | 最大重试次数，默认为 `3`                                                                                                                                                                             |
| Retry.InitialDelay           | String             | 否       | 首次重试时间间隔，默认为 `1s`，重试间隔以会2的倍数递增                                                                                                                                                             |
| Retry.MaxDelay               | String             | 否       | 最大重试时间间隔，默认为 `30s`                                                                                                                                                                         |
| Convert                      | Struct             | 否       | ilogtail数据转换协议配置                                                                                                                                                                           |
| Convert.Protocol             | String             | 否       | ilogtail数据转换协议，可选值：`custom_single`,`influxdb`。默认值：`custom_single`<p>v2版本可选值：`raw`</p>                                                                                                      |
| Convert.Encoding             | String             | 否       | ilogtail flusher数据转换编码，可选值：`json`, `custom`，默认值：`json`                                                                                                                                     |
| Convert.Separator            | String             | 否       | ilogtail数据转换时，PipelineGroupEvents中多个Events之间拼接使用的分隔符。如`\n`。若不设置，则默认不拼接Events，即每个Event作为独立请求向后发送。 默认值为空。<p>当前仅在`Convert.Protocol: raw`有效。</p>      |
| Convert.IgnoreUnExpectedData | Boolean            | 否       | ilogtail数据转换时，遇到非预期的数据的行为，true 跳过，false 报错。默认值 true                                                                                               |
| Convert.TagFieldsRename      | Map<String,String> | 否       | 对日志中tags中的json字段重命名                                                                                                                               |
| Convert.ProtocolFieldsRename | Map<String,String> | 否       | ilogtail日志协议字段重命名，可当前可重命名的字段：`contents`,`tags`和`time`                                                                                             |
| Concurrency                  | Int                | 否       | 向url发起请求的并发数，默认为`1`                                                                                                                               |
| QueueCapacity                  | Int                | 否       | 内部channel的缓存大小，默认为1024      
| AsyncIntercept                  | Boolean                | 否       | 异步过滤数据，默认为否  

## 样例

采集`/home/test-log/`路径下的所有文件名匹配`*.log`规则的文件，并将采集结果以 `custom_single` 协议、`json`格式提交到 `http://localhost:8086/write`。
且提交时，附加 header x-filepath，其值使用log中的 __Tag__:__path__ 的值

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test-log/
    FilePattern: "*.log"
flushers:
  - Type: flusher_http
    RemoteURL: "http://localhost:8086/write"
    Headers:
      x-filepath: "%{tag.__path__}"
    Convert:
      Protocol: custom_single
      Encoding: json
```
