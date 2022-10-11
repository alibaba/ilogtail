# Kafka

## 简介

`flusher_kafka_v2` `flusher`插件可以实现将采集到的数据，经过处理后，发送到Kafka。

## 配置参数

| 参数                                | 类型       | 是否必选 | 说明                                                                                              |
|-----------------------------------|----------|------|-------------------------------------------------------------------------------------------------|
| Type                              | String   | 是    | 插件类型                                                                                            |
| Brokers                           | String数组 | 是    | Kafka Brokers                                                                                   |
| Topic                             | String   | 是    | Kafka Topic,支持动态topic, 例如: test_%{contents.appname}                                             |
| Version                           | String   | 否    | Kafka协议版本号 ,例如：2.0.0                                                                            |
| Protocol                          | String   | 否    | ilogtail数据转换协议，kafka flusher 可选值：custom_single,otlp_log_v1。默认值：custom_single                    |
| Encoding                          | String   | 否    | ilogtail flusher数据转换编码，可选值：json、none、protobuf，默认值：json                                          |
| Authentication                    | Struct   | 否    | Kafka连接访问认证配置，支持SASL/PLAIN，根据kafka服务端认证方式选择配置                                                   |
| Authentication.PlainText.Username | String   | 否    | PlainText认证用户名                                                                                  |
| Authentication.PlainText.Password | String   | 否    | PlainText认证密码                                                                                   |
| Authentication.SASL.Username      | String   | 否    | SASL认证用户名                                                                                       |
| Authentication.SASL.Password      | String   | 否    | SASL认证密码                                                                                        |
| Authentication.Sasl.SaslMechanism | String   | 否    | SASL认证，配置可选项：`PLAIN`、`SCRAM-SHA-256`、`SCRAM-SHA-512`                                            |
| PartitionerType                   | String   | 否    | Partitioner类型。取值：`roundrobin`、`hash`、`random`。默认为：`random`。                                     |
| QequiredAcks                      | int      | 否    | ACK的可靠等级.0=无响应,1=等待本地消息,-1=等待所有副本提交.默认1，                                                        |
| Compression                       | String   | 否    | 压缩算法，可选值：none, snappy，lz4和gzip，默认值none                                                          |
| CompressionLevel                  | Int      | 否    | 压缩级别，可选值：1~9，默认值：4,设置为0则禁用Compression                                                           |
| MaxMessageBytes                   | Int      | 否    | 一个批次提交的大小限制，配置和message.max.bytes对应，默认值：1000000                                                  |
| MaxRetries                        | Int      | 否    | 提交失败重试次数，最大3次，默认值：3                                                                             |
| BulkMaxSize                       | Int      | 否    | 单次请求提交事件数，默认2048                                                                                |
| BulkFlushFrequency                | Int      | 否    | 发送批量 Kafka 请求之前等待的时间,0标识没有时延，默认值0                                                               |
| Timeout                           | Int      | 否    | 等待Kafka brokers响应的超时时间，默认30s                                                                    |
| BrokerTimeout                     | int      | 否    | kafka broker等待请求的最大时长，默认10s                                                                     |
| Metadata.Retry.Max                | int      | 否    | 最大重试次数，默认值：3                                                                                    |
| Metadata.Retry.Backoff            | int      | 否    | 在重试之前等待leader选举发生的时间，默认值：250ms                                                                  |
| Metadata.RefreshFrequency         | int      | 否    | Metadata刷新频率，默认值：250ms                                                                          |
| Metadata.Full                     | int      | 否    | 获取原数数据的策略，获取元数据时使用的策略，当此选项为 true 时，客户端将为所有可用主题维护一整套元数据，如果此选项设置为 false，它将仅刷新已配置主题的元数据。默认值:false。 |
| TagFieldsRename                   | Map      | 否    | 对日志中tags中的json字段重命名                                                                             |
| ProtocolFieldsRename              | Map      | 否    | 对日志的协议字段重命名                                                                                     |
| HashKeys                          | String数组 | 否    | PartitionerType为`hash`时，需指定HashKeys。                                    |
| HashOnce                          | Boolean  | 否    |                                                                                                 |
| ClientID                          | String   | 否    | 写入Kafka的Client ID，默认取值：`LogtailPlugin`。                                                         |

## 样例

采集`/home/test-log/`路径下的所有文件名匹配`*.log`规则的文件，并将采集结果发送到Kafka。

```
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test_log
    FilePattern: "*.log"
flushers:
  - Type: flusher_kafka
    Brokers: 
      - 192.XX.XX.1:9092
      - 192.XX.XX.2:9092
    Topic: KafkaTestTopic
```
## 进阶配置
以下面的一段日志为例，后来将展开介绍ilogtail kafka flusher的一些高阶配置
```
2022-07-22 10:19:23.684 ERROR [springboot-docker] [http-nio-8080-exec-10] com.benchmark.springboot.controller.LogController : error log
```
以上面这行日志为例 , 我们通`ilogtail`的`processor_regex`插件，将上面的日志提取处理后几个关键字段：
- time
- loglevel
- appname
- thread
- class
- message

最后推送到`kafka`的数据样例如下：
```json
{
	"contents": {
		"class": "org.springframework.web.servlet.DispatcherServlet@initServletBean:547",
		"application": "springboot-docker",
		"level": "INFO",
		"message": "Completed initialization in 9 ms",
		"thread": "http-nio-8080-exec-10",
		"time": "2022-07-20 16:55:05.415"
	},
	"tags": {
        "k8s.namespace.name":"java_app",
		"host.ip": "192.168.6.128",
		"host.name": "master",
		"log.file.path": "/data/test.log"
	},
	"time": 1664435098
}
```
### 动态topic
针对上面写入的这种日志格式，如果想根据`application`名称针对不用的应用推送到不通的`topic`，
则`topic`可以这样配置。
```yaml
Topic: test_%{attribute.application}
```
最后`ilogtail`就自动将日志推送到`test_springboot-docker`这个`topic`中。

`topic`动态表达式规则：
- `%{attribute.fieldname}`。`attribute`代表从`contents`中取指定字段值
- `%{tag.fieldname}`,`tag`表示从`tags`中取指定字段值，例如：`%{tag.k8s.namespace.name}`
- 其它方式暂不支持

### TagFieldsRename
例如将`tags`中的`host.name`重命名为`hostname`，配置参考如下：
```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test_log
    FilePattern: "*.log"
flushers:
  - Type: flusher_kafka_v2
    Brokers:
      - 192.XX.XX.1:9092
    TagFieldsRename:
      host.name: hostname
    Topic: KafkaTestTopic
```
### ProtocolFieldsRename
对`ilogtail`协议字段重命名，从上面的日志实例中，设置FlusherMode=1时，
日志主要json格式最外层有三个字段`contents`,`tags`和`time`。
例如在使用Elasticsearch你可能想直接将`time`重命名为`@timestamp`，则配置参考如下：
```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test_log
    FilePattern: "*.log"
flushers:
  - Type: flusher_kafka_v2
    Brokers:
      - 192.XX.XX.1:9092
    TagFieldsRename:
      host.name: hostname
    ProtocolFieldsRename:
      time: '@timestamp'
    Topic: KafkaTestTopic
```
### 指定分区分发
`ilogtail`一共支持三种分区分发方式：
- `random`随机分发, 默认。
- `roundrobin`轮询分发。
- `hash`分发。

`random`和`roundrobin`分发只需要配置`PartitionerType`指定对应的分区分发方式即可。
`hash`分发相对比较特殊，可以指定`HashKeys`，`HashKeys`的中配置的字段名只能是`contents`中的字段属性。

配置用例：
```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test_log
    FilePattern: "*.log"
flushers:
  - Type: flusher_kafka_v2
    PartitionerType: hash
    HashKeys:
      - application  //contents中的`application`
    Brokers:
      - 192.XX.XX.1:9092
    Topic: KafkaTestTopic
```



