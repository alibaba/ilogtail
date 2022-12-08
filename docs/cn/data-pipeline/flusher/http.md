# 标准输出/文件

## 简介

`flusher_http` `flusher`插件可以实现将采集到的数据，经过处理后，通过http格式发送到指定的地址。

## 配置参数

| 参数                         | 类型               | 是否必选 | 说明                                                                                                          |
| ---------------------------- | ------------------ | -------- | ------------------------------------------------------------------------------------------------------------- |
| Type                         | String             | 是       | 插件类型，固定为`flusher_http`                                                                                |
| RemoteURL                    | String             | 是       | 要发送到的URL地址，示例：`http://localhost:8086/write`                                                        |
| Headers                      | Map<String,String> | 否       | 发送时附加的http请求header，如可添加 Authorization、Content-Type等信息                                        |
| Query                        | Map<String,String> | 否       | 发送时附加到url上的query参数，支持动态变量写法，如`{"db":"%{tag.db}"}`                                        |
| Timeout                      | String             | 否       | 请求的超时时间，默认 `60s`                                                                                    |
| Retry.Enable                 | Boolean            | 否       | 是否开启失败重试，默认为 `true`                                                                               |
| Retry.MaxCount               | Int                | 否       | 最大重试次数，默认为 `3`                                                                                      |
| Retry.Delay                  | String             | 否       | 每次重试时间间隔，默认为 `100ms`                                                                              |
| Convert                      | Struct             | 否       | ilogtail数据转换协议配置                                                                                      |
| Convert.Protocol             | String             | 否       | ilogtail数据转换协议，kafka flusher 可选值：`custom_single`,`otlp_log_v1`,`influxdb`。默认值：`custom_single` |
| Convert.Encoding             | String             | 否       | ilogtail flusher数据转换编码，可选值：`json`、`none`、`protobuf`，默认值：`json`                              |
| Convert.TagFieldsRename      | Map<String,String> | 否       | 对日志中tags中的json字段重命名                                                                                |
| Convert.ProtocolFieldsRename | Map<String,String> | 否       | ilogtail日志协议字段重命名，可当前可重命名的字段：`contents`,`tags`和`time`                                   |
| Concurrency                  | Int                | 否       | 向url发起请求的并发数，默认为`1`                                                                              |



## 样例

采集`/home/test-log/`路径下的所有文件名匹配`*.log`规则的文件，并将采集结果以 `custom_single` 协议、`json`格式提交到 `http://localhost:8086/write`。

```
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test-log/
    FilePattern: "*.log"
flushers:
  - Type: flusher_http
    RemoteURL: "http://localhost:8086/write"
    Convert:
      Protocol: custom_single
      Encoding: json
```



