# Open Telemetry gRPC Service Input

## 简介

`service_oltp_grpc` `input`插件实现了`ServiceInputV2`接口，可以接受`Opentelemetry log/metric/trace protocol`的gRPC请求，并且转换输出PipelineGroupEvents。

## 配置参数

| 参数               | 类型      | 是否必选 | 说明                                       |
|-------------------|----------|-------|------------------------------------------|
| Type              | String   | 是    | 插件类型, 固定为`service_oltp_grpc`。                        |
| Address           | String   | 是    |   <p>监听地址。</p>                       |
| MaxRecvMsgSizeMiB    | String | 否    | gRPC Server接收消息的大小上限。                                |
| MaxConcurrentStreams | String   | 否    | gRPC Server 最大并发流。                           |
| ReadBufferSize       | String   | 否    | gRPC Server读缓存大小。 |
| WriteBufferSize      | String   | 否    | gRPC Server写缓存大小。               |


## 样例

采集 

```yaml
enable: true
version: v2
inputs:
  - Type: service_oltp_grpc
    Address: "0.0.0.0:4317"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true  
```