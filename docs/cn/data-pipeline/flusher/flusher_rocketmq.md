# rocketmq

## 简介

`flusher_rocketmq` `flusher`插件可以实现将采集到的数据，经过处理后，发送到rocketmq。

## 版本

[Alpha](../stability-level.md)

## 配置参数

| 参数                         | 类型     | 是否必选 | 说明                                                          |
|----------------------------|--------|------|-------------------------------------------------------------|
| NameServers                | String | 是    | name server address                                         |
| Topic                      | String | 是    | rocketmq Topic,支持动态topic, 例如: `test_%{content.appname}`     |
| Sync                       | bool   | 否    | 默认为异步发送                                                     |
| MaxRetries                 | int    | 否    | 重试次数                                                        |
| BrokerTimeout              | int    | 否    | 超时时间，默认值3s                                                  |
| Convert                    | Struct | 否    | ilogtail数据转换协议配置                                            |
| PartitionerType            | String | 否    | Partitioner类型。取值：`roundrobin`、`hash`、`random`。默认为：`random`。 |
| CompressionLevel           | int    | 否    | 压缩级别，可选值：`0~9`，0速度最快，9压缩效率最高                                |
| CompressMsgBodyOverHowmuch | int    | 否    | 压缩阈值                                                        |
| ProducerGroupName          | String | 否    | 发送者的producer group                                          |


- `NameServers`是个数组，多个`NameServer`地址不能使用`;`或者`,`来隔开放在一行里，`yaml`配置文件中正确的多个`NameServer`地址配置参考如下：

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test_log
    FilePattern: "*.log"
flushers:
  - Type: flusher_rocketmq
    NameServers:
      - 192.XX.XX.1:9876
      - 192.XX.XX.2:9876
      - 192.XX.XX.3:9876
    Topic: accesslog
    Sync: false
```

